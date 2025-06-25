#ifndef AUTH_DEFINED
#define AUTH_DEFINED

#include <common.h>
#include <http.h>

bool check_authentication(BaseConnection & connection, const Request & request);

#endif // AUTH_DEFINED
