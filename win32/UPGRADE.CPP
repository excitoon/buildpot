#include <upgrade.h>

#include <fstream>
#include <string>
#include <utility>

#include <stdio.h>
#include <unistd.h>

#include <windows.h>

#include <auth.h>
#include <common.h>
#include <http.h>
#include <logger.h>
#include <service.h>
#include <utils.h>


bool UpgradeHandler(BaseConnection & connection, const Request & request, void * arg)
{
    if (check_authentication(connection, request))
    {
        return true;
    }
    // FIXME how to use __try, __except?

    StringBoolPair body = connection.read_body(request);
    if (body.second)
    {
        // We can't do much at this point, the connection is already closed.
        // It could theoretically be shut down on sending. FIXME TODO
        return false;
    }
    const string binary_path = get_executable_path();
    StringPair path_parts = path_splitext(binary_path);
    if (lower(path_parts.second) != ".exe")
    {
        Log(HTTP, Error, "Binary path `%s` does not end with `.exe`.", binary_path.c_str());
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }
    string base_ext;
    if (isupper(path_parts.second))
    {
        base_ext = ".EX";
    }
    else
    {
        base_ext = ".ex";
    }
    const string path = path_parts.first + base_ext + "1";
    const string temp_binary_path = path_parts.first + base_ext + "0";
    // data -> EX0, EXE -> EX1, EX0 -> EXE.
    fstream f(path.c_str(), ios_base::out | ios_base::binary);
    if (!f.is_open())
    {
        Log(HTTP, Error, "Failed to open `%s` for writing. GetLastError() -> %d.", path.c_str(), GetLastError());
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }
    f.write(body.first.data(), body.first.size());
    if (!f)
    {
        Log(HTTP, Error, "Failed to write to `%s`. GetLastError() -> %d.", path.c_str(), GetLastError());
        f.close();
        if (remove(path.c_str()) != 0)
        {
            Log(HTTP, Warning, "Failed to remove `%s`. GetLastError() -> %d.", path.c_str(), GetLastError());
        }
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }
    f.close();
    if (exists(temp_binary_path) && remove(temp_binary_path.c_str()) != 0)
    {
        Log(HTTP, Error, "Failed to remove `%s`. GetLastError() -> %d.", temp_binary_path.c_str(), GetLastError());
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }
    if (exists(binary_path) && rename(binary_path.c_str(), temp_binary_path.c_str()) != 0)
    {
        Log(HTTP, Error, "Failed to rename `%s` to `%s`. GetLastError() -> %d.", binary_path.c_str(), temp_binary_path.c_str(), GetLastError());
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }
    if (rename(path.c_str(), binary_path.c_str()) != 0)
    {
        Log(HTTP, Error, "Failed to rename `%s` to `%s`. GetLastError() -> %d.", path.c_str(), binary_path.c_str(), GetLastError());
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }
    Log(HTTP, Info, "Upgrade successful. Restart the service.");
    bool result;
    result = connection.send_response(HTTP_200_OK, "Upgrade successful!\n", "text/plain; charset=utf-8");

    ServiceUpgrade();
    // TODO create service restarter.
    return result;
}
