#ifndef COMMON_DEFINED
#define COMMON_DEFINED

#include <excpt.h>

// C++ refuge.

class bool
{
    int value;

public:
    bool();
    bool(int v);
    bool(const bool & v);
    bool & operator =(const bool & rhs);
    operator int() const;
    bool operator ==(const bool & rhs) const;
    bool operator !=(const bool & rhs) const;
    bool operator &&(const bool & rhs) const;
    bool operator &&(int rhs) const;
    bool operator ||(const bool & rhs) const;
    bool operator ||(int rhs) const;
    bool operator !() const;
};

extern const bool true;
extern const bool false;

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

void ___throw(unsigned long code);
int ___match(unsigned long catch_code, unsigned long exception_code);

#define __throw(e) ___throw(e.code())
#define __catch(e) __except(___match(e::code(), GetExceptionCode()))

#endif // COMMON_DEFINED
