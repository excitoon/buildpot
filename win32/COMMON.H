#ifndef COMMON_DEFINED
#define COMMON_DEFINED

// C++ refuge.

typedef int bool;

#define true 1
#define false 0
#define nullptr 0

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

typedef int ssize_t;

#endif // COMMON_DEFINED
