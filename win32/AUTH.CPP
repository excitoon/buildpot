#include <auth.h>

#include <common.h>
#include <base64.h>
#include <http.h>
#include <logger.h>
#include <utils.h>


bool check_authentication(BaseConnection & connection, const Request & request)
{
    const NameAndValue * auth = request.headers.get("Authorization");
    if (!auth)
    {
        return connection.send_response(HTTP_401_Unauthorized, "Unauthorized\n", "text/plain; charset=utf-8");
    }
    StringTriplet auth_parts = partition(request.headers.get("Authorization")->value, " ");
    if (auth_parts.first != "Basic" || auth_parts.second != " ")
    {
        Log(HTTP, Error, "Invalid `Authorization` header: `%s`.", auth->value.c_str());
        return connection.send_response(HTTP_401_Unauthorized, "Unauthorized\n", "text/plain; charset=utf-8");
    }

    StringWithError decoded = base64_decode(auth_parts.third);
    if (!decoded)
    {
        Log(HTTP, Error, "Failed to decode Base64 string: `%s`.", auth_parts.third.c_str());
        return connection.send_response(HTTP_401_Unauthorized, "Unauthorized\n", "text/plain; charset=utf-8");
    }
    StringTriplet username_password = partition(*decoded, ":");
    if (username_password.second != ":")
    {
        Log(HTTP, Error, "Invalid `Basic` authorization string: `%s`.", auth_parts.third.c_str());
        return connection.send_response(HTTP_401_Unauthorized, "Unauthorized\n", "text/plain; charset=utf-8");
    }
    if (username_password.first != "user" || username_password.third != "password")
    {
        return connection.send_response(HTTP_401_Unauthorized, "Unauthorized\n", "text/plain; charset=utf-8");
    }

    return false;
}
