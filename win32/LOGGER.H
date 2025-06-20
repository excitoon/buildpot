#ifndef LOGGER_DEFINED
#define LOGGER_DEFINED

#include <common.h>


enum Level {
    Fatal = 0,
    Error = 1,
    Warning = 2,
    Info = 3,
    Debug = 4,
    Trace = 5,
};

enum Component {
    Main = 0,
    Service = 1,
    Executer = 2,
    HTTP = 3,
    //JSON = 4,
    Utils = 5,
};

void Log(Component component, Level level, char * format, ...);
bool LoggerInitialize(bool is_service);
void LoggerCleanup();

#endif
