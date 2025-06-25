#ifndef UPLOAD_DEFINED
#define UPLOAD_DEFINED

#include <common.h>
#include <http.h>


bool UploadHandler(BaseConnection & connection, const Request & request, void * arg);

#endif // UPLOAD_DEFINED
