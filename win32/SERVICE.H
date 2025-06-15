#ifndef SERVICE_DEFINED
#define SERVICE_DEFINED

#include <common.h>


extern const char serviceName[];

void ServiceMain(unsigned long dwNumServicesArgs, char ** lpServiceArgVectors);
bool ServiceInitialize();
unsigned long __stdcall ServiceThreadFunc(void * arg);
bool ServiceReportStatus();
void ServiceControl(unsigned long dwCtrlCode);
void ServiceStop();

#endif
