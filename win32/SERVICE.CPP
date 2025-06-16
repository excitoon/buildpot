#include <service.h>

#include <stdio.h>
#include <windows.h>
#include <unistd.h>

#include <base64.h>
#include <common.h>
#include <executer.h>
#include <http.h>
#include <json.h>
#include <logger.h>


SERVICE_STATUS_HANDLE sshStatusHandle;
SERVICE_STATUS ssStatus = { 0 };
HANDLE hThreadHandle = INVALID_HANDLE_VALUE;
HANDLE hServDoneEvent = INVALID_HANDLE_VALUE;
BaseServer * server = nullptr;
bool foreground = true;

const char serviceName[] = "buildpot";


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
            unsigned long serviceThreadID = 0xdeadbeef;
            hThreadHandle = CreateThread(nullptr, 0, ServiceThreadFunc, nullptr, 0, &serviceThreadID);
            if (hThreadHandle != INVALID_HANDLE_VALUE)
            {
                ssStatus.dwCurrentState = SERVICE_RUNNING;
                ServiceReportStatus();

                WaitForSingleObject(hServDoneEvent, INFINITE);
            }
            else
            {
                //ControlService(scHandle, SERVICE_CONTROL_STOP, &ssStatus);
            }
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

bool UpgradeHandler(BaseConnection & connection, const Request & request, void * arg)
{
    // FIXME how to use __try, __except?
    const NameAndValue * auth = request.headers.get("Authorization");
    if (!auth)
    {
        return connection.send_response(HTTP_401_Unauthorized, "Unauthorized\n", "text/plain; charset=utf-8");
    }
    StringTriplet auth_parts = partition(request.headers.get("Authorization")->value, " ");
    if (auth_parts.first != "Basic" || auth_parts.second != " ")
    {
        Log(HTTP, Error, "Invalid Authorization header: `%s`.", auth->value.c_str());
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
        Log(HTTP, Error, "Invalid Basic authorization string: `%s`.", auth_parts.third.c_str());
        return connection.send_response(HTTP_401_Unauthorized, "Unauthorized\n", "text/plain; charset=utf-8");
    }
    if (username_password.first != "user" || username_password.third != "password")
    {
        return connection.send_response(HTTP_401_Unauthorized, "Unauthorized\n", "text/plain; charset=utf-8");
    }

    StringWithError body = connection.read_body(request);
    if (!body)
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
    FILE * fp = fopen(path.c_str(), "wb");
    if (fp == nullptr)
    {
        Log(HTTP, Error, "Failed to open `%s` for writing. GetLastError() -> %d.", path.c_str(), GetLastError());
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }
    if (fwrite(body->data(), 1, body->size(), fp) != body->size())
    {
        Log(HTTP, Error, "Failed to write to `%s`. GetLastError() -> %d.", path.c_str(), GetLastError());
        fclose(fp);
        if (remove(path.c_str()) != 0)
        {
            Log(HTTP, Warning, "Failed to remove `%s`. GetLastError() -> %d.", path.c_str(), GetLastError());
        }
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }
    fclose(fp);
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
    if (foreground)
    {
        server->stop();
        ServiceStop();
    }
    // TODO create service restarter.
    return result;
}

bool ServiceInitialize(bool is_service)
{
    foreground = !is_service;

    Log(Main, Trace, "Executable path is `%s`.", get_executable_path().c_str());
    if (!HTTPInitialize())
    {
        return false;
    }

    server = HTTPStartServer("0.0.0.0", 8981, 10);
    if (server == nullptr)
    {
        Log(Service, Error, "Failed to start HTTP server.");
        return false;
    }
    server->add_route("GET", "/hello", HelloHandler, nullptr);
    server->add_route("POST", "/upgrade", UpgradeHandler, nullptr);
    server->add_route("POST", "/test-json", TestJSONHandler, nullptr);
    server->add_route("POST", "/execute", ExecuteHandler, nullptr);

    hServDoneEvent = CreateEvent(nullptr, true, false, nullptr);
    if (hServDoneEvent == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    return true;
}

void Cleanup()
{
    CloseHandle(hServDoneEvent);
    delete server;
    HTTPCleanup();
    // gzip
    // TODO: handle no memory
    // build long filenames under DOSXNT.
    // TODO: RAII
    // Remove event
    // Remove thread
    // Close status handle
    // Add vector
    // Add deque
    // Add "POST" /upgrade
    // Add basic authentication
    // Ask GPT to implement grpc
}

unsigned long __stdcall ServiceThreadFunc(void * arg)
{
    while (true)
    {
        if (WaitForSingleObject(hServDoneEvent, 1000) == WAIT_OBJECT_0)
        {
            break;
        }
    }
    ExitThread(0);
    return 0;
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
    string key_path = service_key_path();
    key_path += serviceName;
    return key_path;
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

    if (
        RegSetValueEx(key, "DisplayName", 0, REG_SZ, (unsigned char *)serviceName, strlen(serviceName) + 1) == ERROR_SUCCESS
        && RegSetValueEx(key, "ImagePath", 0, REG_EXPAND_SZ, (unsigned char *)image_path.c_str(), image_path.size() + 1) == ERROR_SUCCESS
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
