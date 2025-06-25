#include <upload.h>

#include <fstream>
#include <string>
#include <vector>

#include <stdio.h>

#include <windows.h>

#include <auth.h>
#include <http.h>
#include <logger.h>
#include <service.h>
#include <utils.h>


bool UploadHandler(BaseConnection & connection, const Request & request, void * arg)
{
    if (check_authentication(connection, request))
    {
        return true;
    }

    StringWithError body = connection.read_body(request);
    if (!body)
    {
        return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
    }

    string destination;
    const NameAndValue * dest = request.headers.get("X-Destination-Path");
    if (dest)
    {
        if (dest->value.find(':') != string::npos || dest->value.find("..") != string::npos)
        {
            Log(HTTP, Error, "Invalid `X-Destination-Path` header: `%s`.", dest->value.c_str());
            return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
        }
        destination = dest->value;
    }

    if (destination.size() > 0 && (destination[0] == '/' || destination[0] == '\\'))
    {
        destination = destination.substr(1);
    }

    const string binary_path = get_executable_path();
    StringVector root_parts = path_split(binary_path);
    string path = path_join(root_parts[0], replace(destination, "/", "\\"));

    string dir = dirname(path);
    if (!makedirs(dir, true))
    {
        Log(HTTP, Error, "Failed to create directory `%s`. GetLastError() -> %d.", dir.c_str(), GetLastError());
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }

    fstream f(path.c_str(), ios_base::out | ios_base::binary);
    if (!f.is_open())
    {
        Log(Executer, Error, "Failed to open `%s` for writing. GetLastError() -> %d.", path.c_str(), GetLastError());
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }
    f.write(body->data(), body->size());
    if (!f)
    {
        Log(Executer, Error, "Failed to write to `%s`. GetLastError() -> %d.", path.c_str(), GetLastError());
        remove(path.c_str());
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }

    Log(HTTP, Info, "Saved `%s` of %d bytes size.", path.c_str(), body->size());
    return connection.send_response(HTTP_200_OK, "OK\n", "text/plain; charset=utf-8");
}
