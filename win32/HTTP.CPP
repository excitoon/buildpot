#include <http.h>

#include <string>

#include <ctype.h>
#include <stdlib.h>

#include <winsock.h>

#include <common.h>
#include <logger.h>


const int SD_BOTH = 2;

struct Route
{
    string method;
    string path;
    Handler handler;
    void * arg;

    Route(const string & method, const string & path, Handler handler, void * arg);
};

class Router
{
    Route ** routes;
    size_t routes_size;

    Router(const Router & rhs);
    Router & operator =(const Router & rhs);

public:
    Router();
    ~Router();

    void clear();
    void add(const string & method, const string & path, Handler handler, void * arg);
    const Route * get(const string & path, const string & method) const;
};

class Connection : public BaseConnection
{
public:
    SOCKET socket;
    sockaddr_in address;
    string read_buffer;
    const Router & router;

    Connection(SOCKET socket, const sockaddr_in & address, const Router & router)
        : socket(socket)
        , address(address)
        , router(router)
    {
    }

    int send_all(const string & data, int flags)
    {
        int total = 0;
        int len = data.size();
        const char * buf = data.data();
        while (len > 0)
        {
            int result = send(socket, buf, len, flags);
            if (result == SOCKET_ERROR)
            {
                int error = WSAGetLastError();
                if (error != WSAEINTR)
                {
                    Log(HTTP, Error, "send() of %d bytes size failed with error %d.", len, error);
                    return result;
                }
                else
                {
                    Log(HTTP, Trace, "send() of %d bytes size interrupted by signal.", len);
                }
            }
            else
            {
                buf += result;
                total += result;
                len -= result;
            }
        }
        return total;
    };

    bool read_some()
    {
        // FIXME exception and void
        char buffer[1024];
        int result = recv(socket, buffer, sizeof(buffer), 0);
        if (result > 0)
        {
            read_buffer += string(buffer, result);
            return true;
        }
        else
        {
            int error = WSAGetLastError();
            if (result == SOCKET_ERROR && error == WSAEINTR)
            {
                return true;
            }
            else if (result == 0)
            {
                Log(HTTP, Info, "Connection `%s:%d` closed by peer.", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                return false;
            }
            else
            {
                Log(HTTP, Error, "recv() from `%s:%d` returned %d. WSAGetLastError() -> %d.", inet_ntoa(address.sin_addr), ntohs(address.sin_port), result, error);
                return false;
            }
        }
    }

    StringWithError read_body(const Request & request)
    {
        StringWithError body;
        const char * raw_content_length = request.headers.get("Content-Length")->value.c_str();
        char * raw_content_length_end;
        size_t content_length = strtoul(raw_content_length, &raw_content_length_end, 10);
        if (*raw_content_length_end != '\0' || raw_content_length == raw_content_length_end)
        {
            Log(HTTP, Error, "Invalid `Content-Length` header from `%s:%d`: `%s`", inet_ntoa(address.sin_addr), ntohs(address.sin_port), raw_content_length);
            body.is_error = true;
            return body;
        }
        body = read(content_length);
        return body;
    }

    StringWithError read_line()
    {
        StringWithError line;
        size_t pos = read_buffer.find('\n');
        while (pos == string::npos)
        {
            if (!read_some())
            {
                Log(HTTP, Error, "Failed to read line from `%s:%d`.", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                line.is_error = true;
                return line;
            }
            pos = read_buffer.find('\n');
        }
        line.data = read_buffer.substr(0, pos);
        read_buffer.erase(0, pos + 1);
        return line;
    }

    StringWithError read(size_t len)
    {
        StringWithError data;
        while (read_buffer.size() < len)
        {
            if (!read_some())
            {
                Log(HTTP, Error, "Failed to read %d bytes of data from `%s:%d`.", len, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                data.is_error = true;
                return data;
            }
        }
        data = read_buffer.substr(0, len);
        read_buffer.erase(0, len);
        return data;
    }

    Request read_request()
    {
        Request request;
        StringWithError line = read_line();
        if (!line)
        {
            Log(HTTP, Error, "Failed to read request line from `%s:%d`.", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            request.is_error = true;
            return request;
        }

        size_t pos = line->find(' ');
        if (pos == string::npos)
        {
            Log(HTTP, Error, "Invalid request line from `%s:%d`: %s", inet_ntoa(address.sin_addr), ntohs(address.sin_port), line->c_str());
            request.is_error = true;
            return request;
        }
        request.method = line->substr(0, pos);
        size_t next_pos = line->find(' ', pos + 1);
        if (next_pos == string::npos)
        {
            Log(HTTP, Error, "Invalid request line from `%s:%d`: %s", inet_ntoa(address.sin_addr), ntohs(address.sin_port), line->c_str());
            request.is_error = true;
            return request;
        }
        request.path = line->substr(pos + 1, next_pos - pos - 1);
        request.version = line->substr(next_pos + 1);
        Log(HTTP, Info, "`%s:%d`: %s %s %s", inet_ntoa(address.sin_addr), ntohs(address.sin_port), request.method.data(), request.path.data(), request.version.data());

        while (true)
        {
            line = read_line();
            if (!line)
            {
                Log(HTTP, Error, "Failed to read header line from `%s:%d`.", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                request.is_error = true;
                return request;
            }
            if (!line->empty() && line->back() == '\r')
            {
                line->pop_back();
            }
            if (line->empty())
            {
                break;
            }
            pos = line->find(':');
            if (pos != string::npos)
            {
                string name = line->substr(0, pos);
                string value = line->substr(pos + 1);
                if (!name.empty() && !value.empty())
                {
                    value.erase(0, value.find_first_not_of(" \t"));
                    request.headers.add(name, value);
                }
            }
            else
            {
                Log(HTTP, Warning, "Invalid header line from `%s:%d`: %s", inet_ntoa(address.sin_addr), ntohs(address.sin_port), line->c_str());
                request.is_error = true;
                return request;
            }
        }
        return request;
    }

    bool send_response(unsigned int status_code, const string & text, const string & content_type)
    {
        string reason;
        switch (status_code)
        {
            case 100: reason = "Continue"; break;
            case 101: reason = "Switching Protocols"; break;
            case 102: reason = "Processing"; break;
            case 103: reason = "Early Hints"; break;
            case 200: reason = "OK"; break;
            case 201: reason = "Created"; break;
            case 202: reason = "Accepted"; break;
            case 203: reason = "Non-Authoritative Information"; break;
            case 204: reason = "No Content"; break;
            case 205: reason = "Reset Content"; break;
            case 206: reason = "Partial Content"; break;
            case 207: reason = "Multi-Status"; break;
            case 208: reason = "Already Reported"; break;
            case 226: reason = "IM Used"; break;
            case 300: reason = "Multiple Choices"; break;
            case 301: reason = "Moved Permanently"; break;
            case 302: reason = "Found"; break;
            case 303: reason = "See Other"; break;
            case 304: reason = "Not Modified"; break;
            case 305: reason = "Use Proxy"; break;
            case 306: reason = "Switch Proxy"; break;
            case 307: reason = "Temporary Redirect"; break;
            case 308: reason = "Permanent Redirect"; break;
            case 400: reason = "Bad Request"; break;
            case 401: reason = "Unauthorized"; break;
            case 402: reason = "Payment Required"; break;
            case 403: reason = "Forbidden"; break;
            case 404: reason = "Not Found"; break;
            case 405: reason = "Method Not Allowed"; break;
            case 406: reason = "Not Acceptable"; break;
            case 407: reason = "Proxy Authentication Required"; break;
            case 408: reason = "Request Timeout"; break;
            case 409: reason = "Conflict"; break;
            case 410: reason = "Gone"; break;
            case 411: reason = "Length Required"; break;
            case 412: reason = "Precondition Failed"; break;
            case 413: reason = "Payload Too Large"; break;
            case 414: reason = "URI Too Long"; break;
            case 415: reason = "Unsupported Media Type"; break;
            case 416: reason = "Range Not Satisfiable"; break;
            case 417: reason = "Expectation Failed"; break;
            case 418: reason = "I'm a teapot"; break;
            case 421: reason = "Misdirected Request"; break;
            case 422: reason = "Unprocessable Entity"; break;
            case 423: reason = "Locked"; break;
            case 424: reason = "Failed Dependency"; break;
            case 425: reason = "Too Early"; break;
            case 426: reason = "Upgrade Required"; break;
            case 428: reason = "Precondition Required"; break;
            case 429: reason = "Too Many Requests"; break;
            case 431: reason = "Request Header Fields Too Large"; break;
            case 451: reason = "Unavailable For Legal Reasons"; break;
            case 500: reason = "Internal Server Error"; break;
            case 501: reason = "Not Implemented"; break;
            case 502: reason = "Bad Gateway"; break;
            case 503: reason = "Service Unavailable"; break;
            case 504: reason = "Gateway Timeout"; break;
            case 505: reason = "HTTP Version Not Supported"; break;
            case 506: reason = "Variant Also Negotiates"; break;
            case 507: reason = "Insufficient Storage"; break;
            case 508: reason = "Loop Detected"; break;
            case 510: reason = "Not Extended"; break;
            case 511: reason = "Network Authentication Required"; break;
            default: reason = "Unknown Status"; break;
        }
        string response = "HTTP/1.1 " + to_string(status_code) + " " + reason + "\r\n";
        if (!text.empty())
        {
            response += "Content-Type: " + content_type + "\r\n";
        }
        response += "Content-Length: " + to_string(text.size()) + "\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += text;

        Log(HTTP, Trace, "Sending response to `%s:%d`: %d bytes", inet_ntoa(address.sin_addr), ntohs(address.sin_port), response.size());
        int result = send_all(response, 0);
        if (result == SOCKET_ERROR)
        {
            Log(HTTP, Error, "Failed to send response to `%s:%d`.", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            return false;
        }
        return true;
    }

    static unsigned long __stdcall RecvThreadFunc(Connection * connection)
    {
        Request request = connection->read_request();
        if (request.is_error)
        {
            Log(HTTP, Error, "Failed to read request from `%s:%d`.", inet_ntoa(connection->address.sin_addr), ntohs(connection->address.sin_port));
            shutdown(connection->socket, SD_BOTH);
            closesocket(connection->socket);
            delete connection;
            return 1;
        }

        bool sent_response = false;
        const Route * route = connection->router.get(request.path, request.method);
        if (route != nullptr)
        {
            sent_response = route->handler(*connection, request, route->arg);
        }
        else
        {
            Log(HTTP, Info, "No route found for `%s %s`. Sending 404 response.", request.method.data(), request.path.data());
            sent_response = connection->send_response(404, "Not Found\n", "text/plain; charset=utf-8");
        }
        if (!sent_response)
        {
            Log(HTTP, Error, "Failed to send response to `%s:%d`.", inet_ntoa(connection->address.sin_addr), ntohs(connection->address.sin_port));
        }

        int result = shutdown(connection->socket, SD_BOTH);
        if (result != 0)
        {
            Log(HTTP, Error, "Could not shut down client socket. WSAGetLastError() -> %d.", WSAGetLastError());
        }

        result = closesocket(connection->socket);
        if (result != 0)
        {
            Log(HTTP, Error, "Could not close client socket. WSAGetLastError() -> %d.", WSAGetLastError());
        }

        delete connection;
        return 0;
    }
};

NameAndValue::NameAndValue(const string & name, const string & value)
    : name(Headers::lower(name))
    , value(value)
{
}

string Headers::lower(const string & str)
{
    string result = str;
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = tolower(result[i]);
    }
    return result;
}

Headers::Headers()
{
    headers_size = 0;
}

Headers::Headers(const Headers & rhs)
{
    headers_size = 0;
    *this = rhs;
}

Headers::~Headers()
{
    clear();
}

Headers & Headers::operator =(const Headers & rhs)
{
    if (this != &rhs)
    {
        clear();
        headers_size = rhs.headers_size;
        if (headers_size > 0)
        {
            headers = new NameAndValue *[headers_size];
        }
        for (size_t i = 0; i < headers_size; i++)
        {
            headers[i] = new NameAndValue(*rhs.headers[i]);
        }
    }
    return *this;
}

void Headers::add(const string & name, const string & value)
{
    NameAndValue ** new_headers = new NameAndValue *[headers_size + 1];
    memcpy(new_headers, headers, headers_size * sizeof(NameAndValue *));
    if (headers_size > 0)
    {
        delete[] headers;
    }
    headers = new_headers;
    headers[headers_size] = new NameAndValue(name, value);
    headers_size++;
}

bool Headers::has(const string & name) const
{
    string lower_name = lower(name);
    for (size_t i = 0; i < headers_size; i++)
    {
        if (headers[i]->name == lower_name)
        {
            return true;
        }
    }
    return false;
}

const NameAndValue * Headers::get(const string & name) const
{
    // FIXME exception and &
    string lower_name = lower(name);
    for (size_t i = 0; i < headers_size; i++)
    {
        if (headers[i]->name == lower_name)
        {
            return headers[i];
        }
    }
    return nullptr;
}

void Headers::clear()
{
    for (size_t i = 0; i < headers_size; i++)
    {
        delete headers[i];
    }
    if (headers_size > 0)
    {
        delete[] headers;
    }
    headers_size = 0;
}

size_t Headers::size() const
{
    return headers_size;
}

const NameAndValue & Headers::operator [](size_t index) const
{
    // FIXME exception
    return *headers[index];
}

Request::Request()
    : is_error(false)
{
}

StringWithError::StringWithError()
    : is_error(false)
{
}

StringWithError & StringWithError::operator =(const string & rhs)
{
    data = rhs;
    return *this;
}

string & StringWithError::operator *()
{
    return data;
}

const string & StringWithError::operator *() const
{
    return data;
}

string * StringWithError::operator ->()
{
    return &data;
}

const string * StringWithError::operator ->() const
{
    return &data;
}

StringWithError::operator bool() const
{
    return !is_error;
}

BaseServer::~BaseServer()
{
}

Route::Route(const string & method, const string & path, Handler handler, void * arg)
    : method(method)
    , path(path)
    , handler(handler)
    , arg(arg)
{
}

Router::Router()
{
    routes_size = 0;
}

Router::~Router()
{
    clear();
}

void Router::clear()
{
    for (size_t i = 0; i < routes_size; i++)
    {
        delete routes[i];
    }
    if (routes_size > 0)
    {
        delete[] routes;
    }
    routes_size = 0;
}

void Router::add(const string & method, const string & path, Handler handler, void * arg)
{
    Route ** new_routes = new Route *[routes_size + 1];
    memcpy(new_routes, routes, sizeof(Route *) * routes_size);
    if (routes_size > 0)
    {
        delete[] routes;
    }
    routes = new_routes;
    routes[routes_size] = new Route(method, path, handler, arg);
    ++routes_size;
}

const Route * Router::get(const string & path, const string & method) const
{
    // FIXME exception and & and has
    for (size_t i = 0; i < routes_size; ++i)
    {
        if (routes[i]->path == path && routes[i]->method == method)
        {
            return routes[i];
        }
    }
    return nullptr;
}

class Server : public BaseServer
{
public:
    sockaddr_in address;
    Router router;
    SOCKET serverSocket;

    Server()
        : serverSocket(INVALID_SOCKET)
    {
    }

    ~Server()
    {
    }

    void add_route(const string & method, const string & path, Handler handler, void * arg)
    {
        router.add(method, path, handler, arg);
        Log(HTTP, Info, "Added route for `%s %s`.", method.data(), path.data());
    }

    void stop()
    {
        if (serverSocket != INVALID_SOCKET)
        {
            shutdown(serverSocket, SD_BOTH);
            closesocket(serverSocket);
            serverSocket = INVALID_SOCKET;
        }
    }

    static unsigned long __stdcall AcceptThreadFunc(Server * server)
    {
        while (true)
        {
            sockaddr_in clientAddress;
            int clientAddressSize = sizeof(clientAddress);
            SOCKET clientSocket = accept(server->serverSocket, (sockaddr *)&clientAddress, &clientAddressSize);
            if (clientSocket == INVALID_SOCKET)
            {
                Log(HTTP, Error, "Could not accept client connection. WSAGetLastError() -> %d.", WSAGetLastError());
                break;
            }
            Log(HTTP, Trace, "Accepted client connection from `%s:%d`.", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

            Connection * connection = new Connection(clientSocket, clientAddress, server->router);
            unsigned long clientThreadID = 0xdeadbeef;
            HANDLE hClientThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Connection::RecvThreadFunc, connection, 0, &clientThreadID);
            if (hClientThread == INVALID_HANDLE_VALUE)
            {
                Log(HTTP, Error, "Could not create client thread for `%s:%d`. WSAGetLastError() -> %d.", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port), WSAGetLastError());
                delete connection;
                closesocket(clientSocket);
            }
            else
            {
                CloseHandle(hClientThread);
            }
        }
        delete server;
        return 0;
    }
};

bool HTTPInitialize()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
    {
        Log(HTTP, Error, "Could not initialize sockets. WSAGetLastError() -> %d.", WSAGetLastError());
        return false;
    }
    return true;
}

BaseServer * HTTPStartServer(const string & address, unsigned short port, unsigned int max_connections)
{
    Server * server = new Server();
    if (server == nullptr)
    {
        Log(HTTP, Error, "Could not allocate memory for server.");
        return nullptr;
    }
    server->serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->serverSocket == INVALID_SOCKET)
    {
        Log(HTTP, Error, "Could not create socket. WSAGetLastError() -> %d.", WSAGetLastError());
        delete server;
        return nullptr;
    }

    unsigned long one = 1;
    int error = setsockopt(server->serverSocket, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one));
    if (error != 0)
    {
        Log(HTTP, Error, "Could not set `TCP_NODELAY` on socket. WSAGetLastError() -> %d.", WSAGetLastError());
        delete server;
        return nullptr;
    }

    server->address.sin_family = AF_INET;
    server->address.sin_port = htons(port);
    server->address.sin_addr.s_addr = inet_addr(address.c_str());

    error = bind(server->serverSocket, (sockaddr *)&server->address, sizeof(server->address));
    if (error != 0)
    {
        Log(HTTP, Error, "Could not bind to `%s:%d`. WSAGetLastError() -> %d.", inet_ntoa(server->address.sin_addr), ntohs(server->address.sin_port), WSAGetLastError());
        delete server;
        return nullptr;
    }

    error = listen(server->serverSocket, max_connections);
    if (error != 0)
    {
        Log(HTTP, Error, "Could not listen socket. WSAGetLastError() -> %d.", WSAGetLastError());
        delete server;
        return nullptr;
    }

    unsigned long acceptThreadID = 0xdeadbeef;
    HANDLE hAcceptThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Server::AcceptThreadFunc, server, 0, &acceptThreadID);
    if (hAcceptThread == INVALID_HANDLE_VALUE)
    {
        Log(HTTP, Error, "Could not create accepting thread. WSAGetLastError() -> %d.", WSAGetLastError());
        delete server;
        return nullptr;
    }
    CloseHandle(hAcceptThread);

    Log(HTTP, Info, "Listening on `%s:%d`.", inet_ntoa(server->address.sin_addr), ntohs(server->address.sin_port));
    return server;
}

void HTTPCleanup()
{
    WSACleanup();
}
