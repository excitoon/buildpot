#include <service.h>

#include <cctype>
#include <stdexcept>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <windows.h>

#include <common.h>
#include <download.h>
#include <executer.h>
#include <http.h>
#include <json.h>
#include <logger.h>
#include <upgrade.h>
#include <upload.h>


SERVICE_STATUS_HANDLE sshStatusHandle;
SERVICE_STATUS ssStatus = { 0 };
HANDLE hThreadHandle = INVALID_HANDLE_VALUE;
HANDLE hServDoneEvent = INVALID_HANDLE_VALUE;
BaseServer * server = nullptr;
bool foreground = true;

const char serviceName[] = "buildpot";

StringVector split_command_line(const string & command_line);


// TODO FIXME all of these is a mess
void ServiceMain(unsigned long dwNumServicesArgs, char ** lpServiceArgVectors)
{
    ssStatus.dwServiceType = SERVICE_WIN32;
    ssStatus.dwCurrentState = SERVICE_START_PENDING;
    ssStatus.dwControlsAccepted = 0;
    ssStatus.dwWin32ExitCode = 0; // ERROR_SERVICE_SPECIFIC_ERROR
    ssStatus.dwServiceSpecificExitCode = 0;
    ssStatus.dwCheckPoint = 1;
    ssStatus.dwWaitHint = 20000;

    sshStatusHandle = RegisterServiceCtrlHandler(serviceName, ServiceControl);
    if (sshStatusHandle)
    {
        if (ServiceReportStatus() && ServiceInitialize(false) && ServiceReportStatus())
        {
            ssStatus.dwCurrentState = SERVICE_RUNNING;
            ServiceReportStatus();

            ServiceRun();
        }
    }
    else
    {
        // report error
    }
    ServiceStop();
    {
        if (ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
        {
            ssStatus.dwCurrentState = SERVICE_STOP_PENDING;
            ssStatus.dwCheckPoint = 0;
        }
        ++ssStatus.dwCheckPoint;
        for (int i = 0; i < 5; ++i) // Let user suffer a bit.
        {
            sleep(1);
            ServiceReportStatus();
        }
        // deinit
    }
    ssStatus.dwCurrentState = SERVICE_STOPPED;
    // TODO fill ssStatus.dwWin32ExitCode
    // TODO fill ssStatus.dwServiceSpecificExitCode
    ServiceReportStatus();
}

void ServiceStop()
{
    SetEvent(hServDoneEvent);
}

bool HelloHandler(BaseConnection & connection, const Request & request, void * arg)
{
    return connection.send_response(HTTP_200_OK, "Hello from NT 3.1!\n", "text/plain; charset=utf-8");
}

bool TestJSONHandler(BaseConnection & connection, const Request & request, void * arg)
{
    const NameAndValue * content_type = request.headers.get("Content-Type");
    if (!content_type || content_type->value != "application/json")
    {
        return connection.send_response(HTTP_415_UnsupportedMediaType, "Unsupported Media Type\n", "text/plain; charset=utf-8");
    }
    StringWithError body = connection.read_body(request);
    if (!body)
    {
        return false;
    }
    JSON json = JSON::from_string(*body);
    if (json.type() == JSONError)
    {
        return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
    }
    return connection.send_response(HTTP_200_OK, json.to_string() + "\n", "application/json");
}

bool TestExceptionHandler(BaseConnection & connection, const Request & request, void * arg)
{
    __try
    {
        __throw (logic_error("not implemented"));
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }
    __catch (logic_error)
    {
        return connection.send_response(HTTP_200_OK, "OK\n", "text/plain; charset=utf-8");
    }
}

bool ServiceInitialize(bool is_service)
{
    foreground = !is_service;

    Log(Main, Trace, "Executable path is `%s`.", get_executable_path().c_str());
    if (!HTTPInitialize())
    {
        return false;
    }

    string host = "0.0.0.0";
    unsigned int port = 8981;

    string command_line = GetCommandLine();
    StringVector command_line_parts = split_command_line(command_line);
    for (size_t i = 0; i < command_line_parts.size(); ++i)
    {
        if (command_line_parts[i] == "/host")
        {
            ++i;
            if (i >= command_line_parts.size())
            {
                Log(Main, Error, "Missing value for `/host` argument.");
                return false;
            }
            host = command_line_parts[i];
        }
        else if (command_line_parts[i] == "/port")
        {
            ++i;
            if (i >= command_line_parts.size())
            {
                Log(Main, Error, "Missing value for `/port` argument.");
                return false;
            }
            char * end;
            port = strtoul(command_line_parts[i].c_str(), &end, 10);
            if (*end != '\0' || end == command_line_parts[i].c_str())
            {
                Log(Main, Error, "Invalid value for `/port` argument: `%s`.", command_line_parts[i].c_str());
                return false;
            }
        }
    }

    server = HTTPStartServer(host, port, 10);
    if (server == nullptr)
    {
        Log(Service, Error, "Failed to start HTTP server.");
        return false;
    }
    server->add_route("GET", "/hello", HelloHandler, nullptr);
    server->add_route("POST", "/upgrade", UpgradeHandler, nullptr);
    server->add_route("PUT", "/upload", UploadHandler, nullptr);
    server->add_route("POST", "/download", DownloadHandler, nullptr);
    server->add_route("POST", "/test-json", TestJSONHandler, nullptr);
    server->add_route("POST", "/execute", ExecuteHandler, nullptr);
    server->add_route("GET", "/test-exception", TestExceptionHandler, nullptr);

    hServDoneEvent = CreateEvent(nullptr, true, false, nullptr);
    if (hServDoneEvent == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    return true;
}

void ServiceRun()
{
    WaitForSingleObject(hServDoneEvent, INFINITE);
}

void Cleanup()
{
    CloseHandle(hServDoneEvent);
    delete server;
    HTTPCleanup();
    // extract common files to separate repo
    // check how to implement threading under lower Windows
    // gzip
    // yaml parser
    // TODO: handle no memory
    // build long filenames under DOSXNT.
    // TODO: RAII
    // Remove event
    // Remove thread
    // Close status handle
    // Add deque
    // Implement grpc
    // FIXME find more reliable way to terminate the process - scan children, then terminate them too.
    // FIXME add updater with restarts
    // FIXME it locks because there is too many output
    // FIXME read process list like pview
    // TODO What are LINK32, CL32? Are they better?
    // TODO add yaml support
    // TODO support `deps` attr.
    // use raiseexception everywhere
}

bool ServiceReportStatus()
{
    switch (ssStatus.dwCurrentState)
    {
        case SERVICE_START_PENDING:
        case SERVICE_STOP_PENDING:
            ssStatus.dwControlsAccepted = 0;
            ++ssStatus.dwCheckPoint;
            break;

        case SERVICE_STOPPED:
            ssStatus.dwCheckPoint = 0;
            ssStatus.dwWaitHint = 0;
            break;

        case SERVICE_RUNNING:
            ssStatus.dwControlsAccepted = SERVICE_CONTROL_STOP | SERVICE_CONTROL_PAUSE;
            ssStatus.dwCheckPoint = 0;
            ssStatus.dwWaitHint = 0;
            break;
    }
    return SetServiceStatus(sshStatusHandle, &ssStatus);
}

void ServiceControl(unsigned long dwCtrlCode)
{
    switch (dwCtrlCode)
    {
        case SERVICE_CONTROL_STOP:
            ssStatus.dwCurrentState = SERVICE_STOP_PENDING;
            ssStatus.dwCheckPoint = 1;
            ssStatus.dwWaitHint = 20000;
            break;

        case SERVICE_CONTROL_PAUSE:
            if (ssStatus.dwCurrentState == SERVICE_RUNNING)
            {
                SuspendThread(hThreadHandle);
                ssStatus.dwCurrentState = SERVICE_PAUSED;
            }
            break;

        case SERVICE_CONTROL_CONTINUE:
            if (ssStatus.dwCurrentState == SERVICE_PAUSED)
            {
                ResumeThread(hThreadHandle);
                ssStatus.dwCurrentState = SERVICE_RUNNING;
            }
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        default:
            return;
    }
    ServiceReportStatus();
    if (dwCtrlCode == SERVICE_CONTROL_STOP)
    {
        ServiceStop();
    }
}

string service_key_path()
{
    string key_path = "SYSTEM\\CurrentControlSet\\Services\\";
    key_path += serviceName;
    return key_path;
}

StringVector split_command_line(const string & command_line)
{
    StringVector parts;
    string current;
    bool in_quotes = false;
    size_t i = 0;
    while (i < command_line.size())
    {
        char c = command_line[i];
        if (c == '"')
        {
            if (in_quotes && i + 1 < command_line.size() && command_line[i + 1] == '"')
            {
                current += '"';
                i += 2;
                continue;
            }
            in_quotes = !in_quotes;
            ++i;
        }
        else if (isspace(c) && !in_quotes)
        {
            if (!current.empty())
            {
                parts.push_back(current);
                current.clear();
            }
            ++i;
            while (i < command_line.size() && isspace(command_line[i]) && !in_quotes)
            {
                ++i;
            }
        }
        else
        {
            current += c;
            ++i;
        }
    }
    if (!current.empty())
        parts.push_back(current);
    return parts;
}

bool ServiceInstall()
{
    string key_path = service_key_path();
    HKEY key;
    long res = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        key_path.c_str(),
        0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr);

    if (res != ERROR_SUCCESS)
    {
        return false;
    }

    unsigned long start = 2; // Automatic.
    unsigned long type = 0x20; // Own process.
    unsigned long error_control = 1; // Normal.
    const char empty_multi_sz[2] = { 0, 0 };
    const string image_path = get_executable_path();
    const string user = "LocalSystem";

    string image_path_with_arguments = "\"" + image_path + "\"";
    string command_line = GetCommandLine();
    StringVector command_line_parts = split_command_line(command_line);
    for (size_t i = 1; i < command_line_parts.size(); ++i)
    {
        if (command_line_parts[i] != "/install" && command_line_parts[i] != "/uninstall" && command_line_parts[i] != "/foreground")
        {
            image_path_with_arguments += " \"" + replace(command_line_parts[i], "\"", "\"\"") + "\"";
            continue;
        }
    }

    if (
        RegSetValueEx(key, "DisplayName", 0, REG_SZ, (unsigned char *)serviceName, strlen(serviceName) + 1) == ERROR_SUCCESS
        && RegSetValueEx(key, "ImagePath", 0, REG_EXPAND_SZ, (unsigned char *)image_path_with_arguments.c_str(), image_path_with_arguments.size() + 1) == ERROR_SUCCESS
        && RegSetValueEx(key, "ObjectName", 0, REG_SZ, (unsigned char *)user.c_str(), user.size() + 1) == ERROR_SUCCESS
        && RegSetValueEx(key, "Start", 0, REG_DWORD, (unsigned char *)&start, sizeof(start)) == ERROR_SUCCESS
        && RegSetValueEx(key, "Type", 0, REG_DWORD, (unsigned char *)&type, sizeof(type)) == ERROR_SUCCESS
        && RegSetValueEx(key, "ErrorControl", 0, REG_DWORD, (unsigned char *)&error_control, sizeof(error_control)) == ERROR_SUCCESS
        && RegSetValueEx(key, "DependOnService", 0, REG_MULTI_SZ, (const unsigned char *)empty_multi_sz, sizeof(empty_multi_sz)) == ERROR_SUCCESS
        && RegSetValueEx(key, "DependOnGroup", 0, REG_MULTI_SZ, (const unsigned char *)empty_multi_sz, sizeof(empty_multi_sz)) == ERROR_SUCCESS
    )
    {
        RegCloseKey(key);
        return true;
    }
    else
    {
        RegCloseKey(key);
        return false;
    }
}

bool ServiceUninstall()
{
    string key_path = service_key_path();
    long res = RegDeleteKey(HKEY_LOCAL_MACHINE, key_path.c_str());
    return res == ERROR_SUCCESS;
}

string get_executable_path()
{
    char buffer[1024];
    if (GetModuleFileName(nullptr, buffer, sizeof(buffer)) == sizeof(buffer))
    {
        Log(Main, Error, "Failed to get executable path. GetLastError() -> %d.", GetLastError());
        return string();
    }
    else
    {
        return string(buffer);
    }
}

void ServiceUpgrade()
{
    if (foreground)
    {
        server->stop();
        ServiceStop();
    }
}
