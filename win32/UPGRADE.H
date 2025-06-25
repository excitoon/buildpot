#ifndef UPGRADE_DEFINED
#define UPGRADE_DEFINED

#include <common.h>
#include <http.h>

bool UpgradeHandler(BaseConnection & connection, const Request & request, void * arg);

#endif // UPGRADE_DEFINED
