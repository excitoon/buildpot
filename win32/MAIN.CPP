#include <string>

#include <windows.h>

#include <logger.h>
#include <service.h>


/*
Evolution of deploys:
- 7z x -so ~/cloud/dos8/dos8.raw DL/BUILDPOT/WIN32/BUILDPOT.EXE > ~/cloud/nt31ws/extras/BUILDPOT.EXE && rm -f ~/cloud/nt31ws/extras.iso && hdiutil makehybrid -iso -joliet -o ~/cloud/nt31ws/extras.iso ~/cloud/nt31ws/extras
- 7z x -so ~/cloud/dos8/dos8.raw DL/BUILDPOT/WIN32/BUILDPOT.EXE > ~/cloud/nt31ws/extras/BUILDPOT.EXE && curl -X POST 10.10.10.9:18981/upgrade -H "Authorization: Basic dXNlcjpwYXNzd29yZA==" --data-binary "@$HOME/cloud/nt31ws/extras/BUILDPOT.EXE"
- bazel build ... && curl -X POST 10.10.10.9:18981/upgrade -H "Authorization: Basic dXNlcjpwYXNzd29yZA==" --data-binary @bazel-bin/buildpot.exe
*/

int main(int argc, char ** argv)
{
    bool foreground = false;
    bool install = false;
    bool uninstall = false;

    for (int i = 1; i < argc; ++i)
    {
        if (string(argv[i]) == "/foreground")
        {
            foreground = true;
        }
        else if (string(argv[i]) == "/install")
        {
            foreground = true;
            install = true;
        }
        else if (string(argv[i]) == "/uninstall")
        {
            foreground = true;
            uninstall = true;
        }
    }

    LoggerInitialize(!foreground);
    if (install)
    {
        if (ServiceInstall())
        {
            Log(Main, Info, "Service installed successfully.");
            return 0;
        }
        else
        {
            Log(Main, Error, "Failed to install service. GetLastError -> %d.", GetLastError());
            return 1;
        }
    }
    else if (uninstall)
    {
        if (ServiceUninstall())
        {
            Log(Main, Info, "Service uninstalled successfully.");
            return 0;
        }
        else
        {
            Log(Main, Error, "Failed to uninstall service. GetLastError -> %d.", GetLastError());
            return 1;
        }
    }
    else if (foreground)
    {
        Log(Main, Trace, "sizeof(char) is %d.", sizeof(char)); // 1.
        Log(Main, Trace, "sizeof(int) is %d.", sizeof(int)); // 4.
        Log(Main, Trace, "sizeof(long) is %d.", sizeof(long)); // 4.

        ServiceInitialize(!foreground);
        ServiceThreadFunc(nullptr);
        return 0;
    }
    else
    {
        SERVICE_TABLE_ENTRY dispatchTable[] =
        {
            { (char *)serviceName, ServiceMain },
            { nullptr, nullptr },
        };

        if (StartServiceCtrlDispatcher(dispatchTable))
        {
            return 0;
        }
        else
        {
            ServiceStop();
            return 1;
        }
    }
}
