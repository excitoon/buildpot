#include <common.h>

#include <excpt.h>

#include <windows.h>


const bool true = bool(1);
const bool false = bool();

bool::bool()
    : value(0)
{
}

bool::bool(int v)
    : value(v ? 1 : 0)
{
}

bool::bool(const bool & v)
    : value(v.value)
{
}

bool & bool::operator =(const bool & rhs)
{
    value = rhs.value;
    return *this;
}

bool::operator int() const
{
    return value;
}

bool bool::operator ==(const bool & rhs) const
{
    return value == rhs.value;
}

bool bool::operator !=(const bool & rhs) const
{
    return value != rhs.value;
}

bool bool::operator &&(const bool & rhs) const
{
    return value != 0 && rhs.value != 0;
}

bool bool::operator &&(int rhs) const
{
    return value != 0 && rhs != 0;
}

bool bool::operator ||(const bool & rhs) const
{
    return value != 0 || rhs.value != 0;
}

bool bool::operator ||(int rhs) const
{
    return value != 0 || rhs != 0;
}

bool bool::operator !() const
{
    return value == 0;
}

void ___throw(unsigned long code)
{
    RaiseException(code, 0, 0, nullptr);
}

int ___match(unsigned long catch_code, unsigned long exception_code)
{
    return catch_code == exception_code ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}
