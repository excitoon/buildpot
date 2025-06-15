#ifndef HTTP_DEFINED
#define HTTP_DEFINED

#include <string>

#include <common.h>
#include <utils.h>


enum HTTPStatusCode {
    HTTP_100_Continue = 100,
    HTTP_101_SwitchingProtocols = 101,
    HTTP_102_Processing = 102,
    HTTP_103_EarlyHints = 103,
    HTTP_200_OK = 200,
    HTTP_201_Created = 201,
    HTTP_202_Accepted = 202,
    HTTP_203_NonAuthoritativeInformation = 203,
    HTTP_204_NoContent = 204,
    HTTP_205_ResetContent = 205,
    HTTP_206_PartialContent = 206,
    HTTP_207_MultiStatus = 207,
    HTTP_208_AlreadyReported = 208,
    HTTP_226_IMUsed = 226,
    HTTP_300_MultipleChoices = 300,
    HTTP_301_MovedPermanently = 301,
    HTTP_302_Found = 302,
    HTTP_303_SeeOther = 303,
    HTTP_304_NotModified = 304,
    HTTP_305_UseProxy = 305,
    HTTP_306_SwitchProxy = 306,
    HTTP_307_TemporaryRedirect = 307,
    HTTP_308_PermanentRedirect = 308,
    HTTP_400_BadRequest = 400,
    HTTP_401_Unauthorized = 401,
    HTTP_402_PaymentRequired = 402,
    HTTP_403_Forbidden = 403,
    HTTP_404_NotFound = 404,
    HTTP_405_MethodNotAllowed = 405,
    HTTP_406_NotAcceptable = 406,
    HTTP_407_ProxyAuthenticationRequired = 407,
    HTTP_408_RequestTimeout = 408,
    HTTP_409_Conflict = 409,
    HTTP_410_Gone = 410,
    HTTP_411_LengthRequired = 411,
    HTTP_412_PreconditionFailed = 412,
    HTTP_413_PayloadTooLarge = 413,
    HTTP_414_URITooLong = 414,
    HTTP_415_UnsupportedMediaType = 415,
    HTTP_416_RangeNotSatisfiable = 416,
    HTTP_417_ExpectationFailed = 417,
    HTTP_418_ImATeapot = 418,
    HTTP_419_ImABuildpot = 419,
    HTTP_420_ItsATeatime = 420,
    HTTP_421_MisdirectedRequest = 421,
    HTTP_422_UnprocessableEntity = 422,
    HTTP_423_Locked = 423,
    HTTP_424_FailedDependency = 424,
    HTTP_425_TooEarly = 425,
    HTTP_426_UpgradeRequired = 426,
    HTTP_428_PreconditionRequired = 428,
    HTTP_429_TooManyRequests = 429,
    HTTP_431_RequestHeaderFieldsTooLarge = 431,
    HTTP_451_UnavailableForLegalReasons = 451,
    HTTP_500_InternalServerError = 500,
    HTTP_501_NotImplemented = 501,
    HTTP_502_BadGateway = 502,
    HTTP_503_ServiceUnavailable = 503,
    HTTP_504_GatewayTimeout = 504,
    HTTP_505_HTTPVersionNotSupported = 505,
    HTTP_506_VariantAlsoNegotiates = 506,
    HTTP_507_InsufficientStorage = 507,
    HTTP_508_LoopDetected = 508,
    HTTP_510_NotExtended = 510,
    HTTP_511_NetworkAuthenticationRequired = 511
};

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

string HTTPGetReason(unsigned int status_code);

#endif
