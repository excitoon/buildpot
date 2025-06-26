#include <http.h>

#include <string>
#include <utility>
#include <vector>

#include <stdlib.h>

#include <winsock.h>

#include <common.h>
#include <logger.h>
#include <utils.h>


const int SD_BOTH = 2;

class Router
{
    RouteVector routes;

public:
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

    StringBoolPair read_body(const Request & request)
    {
        StringBoolPair body;
        const string * content_length_header = request.headers.get("Content-Length");
        if (content_length_header == nullptr)
        {
            Log(HTTP, Error, "Missing `Content-Length` header from `%s:%d`.", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            body.second = true;
            return body;
        }
        const char * raw_content_length = content_length_header->c_str();
        char * raw_content_length_end;
        size_t content_length = strtoul(raw_content_length, &raw_content_length_end, 10);
        if (*raw_content_length_end != '\0' || raw_content_length == raw_content_length_end)
        {
            Log(HTTP, Error, "Invalid `Content-Length` header from `%s:%d`: `%s`.", inet_ntoa(address.sin_addr), ntohs(address.sin_port), raw_content_length);
            body.second = true;
            return body;
        }
        return read(content_length);
    }

    StringBoolPair read_line()
    {
        StringBoolPair line;
        size_t pos = read_buffer.find('\n');
        while (pos == string::npos)
        {
            if (!read_some())
            {
                Log(HTTP, Error, "Failed to read line from `%s:%d`.", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                line.second = true;
                return line;
            }
            pos = read_buffer.find('\n');
        }
        line.first = read_buffer.substr(0, pos);
        read_buffer.erase(0, pos + 1);
        return line;
    }

    StringBoolPair read(size_t len)
    {
        StringBoolPair data;
        while (read_buffer.size() < len)
        {
            if (!read_some())
            {
                Log(HTTP, Error, "Failed to read %d bytes of data from `%s:%d`.", len, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                data.second = true;
                return data;
            }
        }
        data.first = read_buffer.substr(0, len);
        read_buffer.erase(0, len);
        return data;
    }

    Request read_request()
    {
        Request request;
        StringBoolPair line = read_line();
        if (line.second)
        {
            Log(HTTP, Error, "Failed to read request line from `%s:%d`.", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            request.is_error = true;
            return request;
        }

        size_t pos = line.first.find(' ');
        if (pos == string::npos)
        {
            Log(HTTP, Error, "Invalid request line from `%s:%d`: %s.", inet_ntoa(address.sin_addr), ntohs(address.sin_port), line.first.c_str());
            request.is_error = true;
            return request;
        }
        request.method = line.first.substr(0, pos);
        size_t next_pos = line.first.find(' ', pos + 1);
        if (next_pos == string::npos)
        {
            Log(HTTP, Error, "Invalid request line from `%s:%d`: %s.", inet_ntoa(address.sin_addr), ntohs(address.sin_port), line.first.c_str());
            request.is_error = true;
            return request;
        }
        request.path = line.first.substr(pos + 1, next_pos - pos - 1);
        request.version = line.first.substr(next_pos + 1);
        Log(HTTP, Info, "`%s:%d`: %s %s %s.", inet_ntoa(address.sin_addr), ntohs(address.sin_port), request.method.data(), request.path.data(), request.version.data());

        while (true)
        {
            line = read_line();
            if (line.second)
            {
                Log(HTTP, Error, "Failed to read header line from `%s:%d`.", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                request.is_error = true;
                return request;
            }
            if (!line.first.empty() && line.first.back() == '\r')
            {
                line.first.pop_back();
            }
            if (line.first.empty())
            {
                break;
            }
            pos = line.first.find(':');
            if (pos != string::npos)
            {
                string name = line.first.substr(0, pos);
                string value = line.first.substr(pos + 1);
                if (!name.empty() && !value.empty())
                {
                    value.erase(0, value.find_first_not_of(" \t"));
                    request.headers.add(name, value);
                }
            }
            else
            {
                Log(HTTP, Warning, "Invalid header line from `%s:%d`: %s.", inet_ntoa(address.sin_addr), ntohs(address.sin_port), line.first.c_str());
                request.is_error = true;
                return request;
            }
        }
        return request;
    }

    bool send_response(unsigned int status_code, const string & text, const string & content_type)
    {
        string reason = HTTPGetReason(status_code);
        string response = "HTTP/1.1 " + to_string(status_code) + " " + reason + "\r\n";
        if (!text.empty())
        {
            response += "Content-Type: " + content_type + "\r\n";
        }
        response += "Content-Length: " + to_string(text.size()) + "\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += text;

        Log(HTTP, Trace, "Sending response to `%s:%d`: %d bytes.", inet_ntoa(address.sin_addr), ntohs(address.sin_port), response.size());
        int result = send_all(response, 0);
        if (result == SOCKET_ERROR)
        {
            Log(HTTP, Error, "Failed to send response to `%s:%d`.", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            return false;
        }
        return true;
    }

    bool send_response(unsigned int status_code, const StringSharedPtr & text, const string & content_type)
    {
        if (!text)
        {
            return send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
        }
        else
        {
            return send_response(status_code, *text, content_type);
        }
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
            sent_response = connection->send_response(HTTP_404_NotFound, "Not Found\n", "text/plain; charset=utf-8");
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

void Headers::add(const string & name, const string & value)
{
    headers.push_back(StringPair(lower(name), value));
}

bool Headers::has(const string & name) const
{
    string lower_name = lower(name);
    for (size_t i = 0; i < headers.size(); ++i)
    {
        if (headers[i].first == lower_name)
        {
            return true;
        }
    }
    return false;
}

const string * Headers::get(const string & name) const
{
    string lower_name = lower(name);
    for (size_t i = 0; i < headers.size(); ++i)
    {
        if (headers[i].first == lower_name)
        {
            return &headers[i].second;
        }
    }
    return nullptr;
}

void Headers::clear()
{
    headers.clear();
}

size_t Headers::size() const
{
    return headers.size();
}

const StringPair & Headers::operator [](size_t index) const
{
    return headers[index];
}

Request::Request()
    : is_error(false)
{
}

BaseServer::~BaseServer()
{
}

Route::Route()
{
}

Route::Route(const string & method, const string & path, Handler handler, void * arg)
    : method(method)
    , path(path)
    , handler(handler)
    , arg(arg)
{
}

void Router::add(const string & method, const string & path, Handler handler, void * arg)
{
    routes.push_back(Route(method, path, handler, arg));
}

const Route * Router::get(const string & path, const string & method) const
{
    for (size_t i = 0; i < routes.size(); ++i)
    {
        if (routes[i].path == path && routes[i].method == method)
        {
            return &routes[i];
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

string HTTPGetReason(unsigned int status_code)
{
    switch (status_code)
    {
        case HTTP_100_Continue:                      return "Continue";
        case HTTP_101_SwitchingProtocols:            return "Switching Protocols";
        case HTTP_102_Processing:                    return "Processing";
        case HTTP_103_EarlyHints:                    return "Early Hints";
        case HTTP_200_OK:                            return "OK";
        case HTTP_201_Created:                       return "Created";
        case HTTP_202_Accepted:                      return "Accepted";
        case HTTP_203_NonAuthoritativeInformation:   return "Non-Authoritative Information";
        case HTTP_204_NoContent:                     return "No Content";
        case HTTP_205_ResetContent:                  return "Reset Content";
        case HTTP_206_PartialContent:                return "Partial Content";
        case HTTP_207_MultiStatus:                   return "Multi-Status";
        case HTTP_208_AlreadyReported:               return "Already Reported";
        case HTTP_226_IMUsed:                        return "IM Used";
        case HTTP_300_MultipleChoices:               return "Multiple Choices";
        case HTTP_301_MovedPermanently:              return "Moved Permanently";
        case HTTP_302_Found:                         return "Found";
        case HTTP_303_SeeOther:                      return "See Other";
        case HTTP_304_NotModified:                   return "Not Modified";
        case HTTP_305_UseProxy:                      return "Use Proxy";
        case HTTP_306_SwitchProxy:                   return "Switch Proxy";
        case HTTP_307_TemporaryRedirect:             return "Temporary Redirect";
        case HTTP_308_PermanentRedirect:             return "Permanent Redirect";
        case HTTP_400_BadRequest:                    return "Bad Request";
        case HTTP_401_Unauthorized:                  return "Unauthorized";
        case HTTP_402_PaymentRequired:               return "Payment Required";
        case HTTP_403_Forbidden:                     return "Forbidden";
        case HTTP_404_NotFound:                      return "Not Found";
        case HTTP_405_MethodNotAllowed:              return "Method Not Allowed";
        case HTTP_406_NotAcceptable:                 return "Not Acceptable";
        case HTTP_407_ProxyAuthenticationRequired:   return "Proxy Authentication Required";
        case HTTP_408_RequestTimeout:                return "Request Timeout";
        case HTTP_409_Conflict:                      return "Conflict";
        case HTTP_410_Gone:                          return "Gone";
        case HTTP_411_LengthRequired:                return "Length Required";
        case HTTP_412_PreconditionFailed:            return "Precondition Failed";
        case HTTP_413_PayloadTooLarge:               return "Payload Too Large";
        case HTTP_414_URITooLong:                    return "URI Too Long";
        case HTTP_415_UnsupportedMediaType:          return "Unsupported Media Type";
        case HTTP_416_RangeNotSatisfiable:           return "Range Not Satisfiable";
        case HTTP_417_ExpectationFailed:             return "Expectation Failed";
        case HTTP_418_ImATeapot:                     return "I'm a teapot";
        case HTTP_419_ImABuildpot:                   return "I'm a buildpot";
        case HTTP_420_ItsATeatime:                   return "It's a teatime";
        case HTTP_421_MisdirectedRequest:            return "Misdirected Request";
        case HTTP_422_UnprocessableEntity:           return "Unprocessable Entity";
        case HTTP_423_Locked:                        return "Locked";
        case HTTP_424_FailedDependency:              return "Failed Dependency";
        case HTTP_425_TooEarly:                      return "Too Early";
        case HTTP_426_UpgradeRequired:               return "Upgrade Required";
        case HTTP_428_PreconditionRequired:          return "Precondition Required";
        case HTTP_429_TooManyRequests:               return "Too Many Requests";
        case HTTP_431_RequestHeaderFieldsTooLarge:   return "Request Header Fields Too Large";
        case HTTP_451_UnavailableForLegalReasons:    return "Unavailable For Legal Reasons";
        case HTTP_500_InternalServerError:           return "Internal Server Error";
        case HTTP_501_NotImplemented:                return "Not Implemented";
        case HTTP_502_BadGateway:                    return "Bad Gateway";
        case HTTP_503_ServiceUnavailable:            return "Service Unavailable";
        case HTTP_504_GatewayTimeout:                return "Gateway Timeout";
        case HTTP_505_HTTPVersionNotSupported:       return "HTTP Version Not Supported";
        case HTTP_506_VariantAlsoNegotiates:         return "Variant Also Negotiates";
        case HTTP_507_InsufficientStorage:           return "Insufficient Storage";
        case HTTP_508_LoopDetected:                  return "Loop Detected";
        case HTTP_510_NotExtended:                   return "Not Extended";
        case HTTP_511_NetworkAuthenticationRequired: return "Network Authentication Required";
        default:                                     return "Unknown Status";
    }
}
