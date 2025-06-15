#ifndef SERVICE_DEFINED
#define SERVICE_DEFINED

#include <common.h>


extern const char serviceName[];

void ServiceMain(unsigned long dwNumServicesArgs, char ** lpServiceArgVectors);
bool ServiceInitialize(bool is_service);
unsigned long __stdcall ServiceThreadFunc(void * arg);
bool ServiceReportStatus();
void ServiceControl(unsigned long dwCtrlCode);
void ServiceStop();
bool ServiceInstall();
bool ServiceUninstall();

#endif
