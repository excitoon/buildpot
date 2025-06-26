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


bool DownloadHandler(BaseConnection & connection, const Request & request, void * arg)
{
    if (check_authentication(connection, request))
    {
        return true;
    }

    const string * src = request.headers.get("X-Source-Path");
    if (src == nullptr || src->find(':') != string::npos || src->find("..") != string::npos)
    {
        Log(HTTP, Error, "Invalid `X-Source-Path` header: `%s`.", src->c_str());
        return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
    }
    string source = *src;
    if (source.size() > 0 && (source[0] == '/' || source[0] == '\\'))
    {
        source = source.substr(1);
    }

    const string binary_path = get_executable_path();
    StringVector root_parts = path_split(binary_path);
    string path = path_join(root_parts[0], replace(source, "/", "\\"));

    fstream f(path.c_str(), ios_base::in | ios_base::binary);
    if (!f.is_open())
    {
        Log(HTTP, Error, "Failed to open `%s` for reading. GetLastError() -> %d.", path.c_str(), GetLastError());
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }

    string body;
    f.seekg(0, ios_base::end);
    body.resize(f.tellg());
    f.seekg(0, ios_base::beg);
    f.read(body.data(), body.size());

    if (!f)
    {
        Log(HTTP, Error, "Failed to read from `%s`. GetLastError() -> %d.", path.c_str(), GetLastError());
    }

    Log(HTTP, Trace, "Sending `%s` of %d bytes size.", path.c_str(), body.size());
    return connection.send_response(HTTP_200_OK, body, "application/octet-stream");
}
