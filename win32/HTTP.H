#ifndef HTTP_DEFINED
#define HTTP_DEFINED

#include <string>

#include <common.h>


class NameAndValue
{
public:
    string name;
    string value;

    NameAndValue(const string & name, const string & value);
};

class Headers
{
    NameAndValue ** headers;
    size_t headers_size;

public:
    Headers();
    Headers(const Headers & rhs);
    ~Headers();

    static string lower(const string & str);

    Headers & operator =(const Headers & rhs);

    void add(const string & name, const string & value);
    bool has(const string & name) const;
    const NameAndValue * get(const string & name) const;
    void clear();
    size_t size() const;
    const NameAndValue & operator [](size_t index) const;
};

struct Request
{
    string method;
    string path;
    string version;
    Headers headers;
    bool is_error;

    Request();
};

struct StringWithError
{
    string data;
    bool is_error;

    StringWithError & operator =(const string & rhs);
    const string & operator *() const;
    string & operator *();
    const string * operator ->() const;
    string * operator ->();
    operator bool() const;

    StringWithError();
};

class BaseConnection
{
public:
    virtual StringWithError read_body(const Request & request) = 0;
    virtual StringWithError read_line() = 0;
    virtual StringWithError read(size_t len) = 0;
    virtual bool send_response(unsigned int status_code, const string & text, const string & content_type = "text/plain") = 0;
};

typedef bool (* Handler)(BaseConnection & connection, const Request & request, void * arg);

class BaseServer
{
public:
    virtual void add_route(const string & method, const string & path, Handler handler, void * arg) = 0;
    virtual void stop() = 0;
    virtual ~BaseServer();
};

class Connection;

bool HTTPInitialize();
BaseServer * HTTPStartServer(const string & address, unsigned short port, unsigned int max_connections = 5);
void HTTPCleanup();

#endif
