#include <logger.h>

#include <string>

#include <stdio.h>

#include <windows.h>

#include <common.h>
#include <service.h>


HANDLE hEventSource = INVALID_HANDLE_VALUE;
bool service;


bool LoggerInitialize(bool is_service)
{
    service = is_service;
    if (service)
    {
        hEventSource = RegisterEventSource(nullptr, serviceName);
        if (hEventSource == INVALID_HANDLE_VALUE)
        {
            return false;
        }
    }
    return true;
}

void LoggerCleanup()
{
    if (service)
    {
        DeregisterEventSource(hEventSource);
    }
}

void Log(Component component, Level level, char * fmt, ...)
{
    if (service)
    {
        char lines[4][256];
        char * strings[sizeof(lines)/sizeof(lines[0])];
        for (int i = 0; i < sizeof(strings)/sizeof(strings[0]); ++i)
        {
            strings[i] = &lines[i][0];
        }

        va_list valist;
        va_start(valist, fmt);
        _vsnprintf(strings[0], 255, fmt, valist);
        va_end(valist);
        strings[1][0] = '\0';
        _snprintf(strings[2], 255, "Level: %d", level);
        _snprintf(strings[3], 255, "Component: %d", component);

        unsigned type = EVENTLOG_INFORMATION_TYPE;
        switch (level) {
            case Fatal:
            case Error:
                type = EVENTLOG_ERROR_TYPE;
            case Warning:
                type = EVENTLOG_WARNING_TYPE;
        }
        unsigned category = 0;
        const unsigned long event_id = 0;

        ReportEvent(hEventSource, type, category, event_id, nullptr, sizeof(strings)/sizeof(strings[0]), 0, (const char **)&strings[0], nullptr);
    }
    else
    {
        char message[512];
        va_list valist;
        va_start(valist, fmt);
        _vsnprintf(message, 511, fmt, valist);
        va_end(valist);
        printf("Level: %d | Component: %d | %s\n", level, component, message);
    }
}
