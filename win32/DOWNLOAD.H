#ifndef DOWNLOAD_DEFINED
#define DOWNLOAD_DEFINED

#include <common.h>
#include <http.h>

bool DownloadHandler(BaseConnection & connection, const Request & request, void * arg);

#endif // DOWNLOAD_DEFINED
