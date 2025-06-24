#ifndef SERVICE_DEFINED
#define SERVICE_DEFINED

#include <common.h>


extern const char serviceName[];

void ServiceMain(unsigned long dwNumServicesArgs, char ** lpServiceArgVectors);
bool ServiceInitialize(bool is_service);
bool ServiceReportStatus();
void ServiceControl(unsigned long dwCtrlCode);
void ServiceStop();
void ServiceRun();
bool ServiceInstall();
bool ServiceUninstall();

class string;
string get_executable_path();

#endif
