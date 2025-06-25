#ifndef EXECUTER_DEFINED
#define EXECUTER_DEFINED

#include <common.h>
#include <http.h>

bool ExecuteHandler(BaseConnection & connection, const Request & request, void * arg);

#endif // EXECUTER_DEFINED
