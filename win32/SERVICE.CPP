#include <service.h>

#include <stdio.h>
#include <windows.h>

#include <common.h>
#include <http.h>
#include <logger.h>
#include <base64.h>


SERVICE_STATUS_HANDLE sshStatusHandle;
SERVICE_STATUS ssStatus = { 0 };
HANDLE hThreadHandle = INVALID_HANDLE_VALUE;
HANDLE hServDoneEvent = INVALID_HANDLE_VALUE;
BaseServer * server = nullptr;

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
        if (ServiceReportStatus() && ServiceInitialize() && ServiceReportStatus())
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
            Sleep(1000);
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
    return connection.send_response(200, "Hello from NT 3.1!\n", "text/plain; charset=utf-8");
}

bool UpgradeHandler(BaseConnection & connection, const Request & request, void * arg)
{
    // FIXME how to use __try, __except?
    const NameAndValue * auth = request.headers.get("Authorization");
    if (auth)
    {
        if (auth->value.substr(0, 6) != "Basic ")
        {
            return connection.send_response(401, "Unauthorized\n", "text/plain; charset=utf-8");
        }
        string decoded = base64_decode(auth->value.substr(6));
        size_t colon_pos = decoded.find(':');
        if (colon_pos == string::npos || colon_pos == 0 || colon_pos == decoded.size() - 1)
        {
            return connection.send_response(401, "Unauthorized\n", "text/plain; charset=utf-8");
        }
        string username = decoded.substr(0, colon_pos);
        string password = decoded.substr(colon_pos + 1);
        if (username != "user" || password != "password")
        {
            return connection.send_response(401, "Unauthorized\n", "text/plain; charset=utf-8");
        }

        StringWithError body = connection.read_body(request);
        if (!body)
        {
            // We can't do much at this point, the connection is already closed.
            // It could theoretically be shut down on sending. FIXME TODO
            return false;
        }
        const string path = "C:\\BUILDPOT.EX1";
        const string binary_path = "C:\\BUILDPOT.EXE";
        const string temp_binary_path = "C:\\BUILDPOT.EX0";
        FILE * fp = fopen(path.c_str(), "wb");
        if (fp == nullptr)
        {
            Log(HTTP, Error, "Failed to open `%s` for writing. GetLastError() -> %d.", path.c_str(), GetLastError());
            return connection.send_response(500, "Internal Server Error\n", "text/plain; charset=utf-8");
        }
        if (fwrite(body->data(), 1, body->size(), fp) != body->size())
        {
            Log(HTTP, Error, "Failed to write to `%s`. GetLastError() -> %d.", path.c_str(), GetLastError());
            fclose(fp);
            if (remove(path.c_str()) != 0) {
                Log(HTTP, Warning, "Failed to remove `%s`. GetLastError() -> %d.", path.c_str(), GetLastError());
            }
            return connection.send_response(500, "Internal Server Error\n", "text/plain; charset=utf-8");
        }
        fclose(fp);
        fp = fopen(temp_binary_path.c_str(), "rb");
        if (fp != nullptr)
        {
            fclose(fp);
            if (remove(temp_binary_path.c_str()) != 0) {
                Log(HTTP, Error, "Failed to remove `%s`. GetLastError() -> %d.", temp_binary_path.c_str(), GetLastError());
                return connection.send_response(500, "Internal Server Error\n", "text/plain; charset=utf-8");
            }
        }
        fp = fopen(binary_path.c_str(), "rb");
        if (fp != nullptr)
        {
            fclose(fp);
            if (rename(binary_path.c_str(), temp_binary_path.c_str()) != 0) {
                Log(HTTP, Error, "Failed to rename `%s` to `%s`. GetLastError() -> %d.", binary_path.c_str(), temp_binary_path.c_str(), GetLastError());
                return connection.send_response(500, "Internal Server Error\n", "text/plain; charset=utf-8");
            }
        }
        if (rename(path.c_str(), binary_path.c_str()) != 0) {
            Log(HTTP, Error, "Failed to rename `%s` to `%s`. GetLastError() -> %d.", path.c_str(), binary_path.c_str(), GetLastError());
            return connection.send_response(500, "Internal Server Error\n", "text/plain; charset=utf-8");
        }
        Log(HTTP, Info, "Upgrade successful. Restart the service.");
        bool result = connection.send_response(200, "Upgrade successful!\n", "text/plain; charset=utf-8");
        server->stop();
        ServiceStop();
        return result;
    }
    else
    {
        return connection.send_response(401, "Unauthorized\n", "text/plain; charset=utf-8");
    }
}

bool ServiceInitialize()
{
    if (!HTTPInitialize())
    {
        return false;
    }

    server = HTTPStartServer("0.0.0.0", 8980, 10);
    if (server == nullptr)
    {
        Log(Service, Error, "Failed to start HTTP server.");
        return false;
    }
    server->add_route("GET", "/hello", HelloHandler, nullptr);
    server->add_route("POST", "/upgrade", UpgradeHandler, nullptr);

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
    // TODO: handle no memory
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
