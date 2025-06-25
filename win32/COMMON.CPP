#include <common.h>

#include <excpt.h>

#include <windows.h>


void ___throw(unsigned long code)
{
    RaiseException(code, 0, 0, nullptr);
}

int ___match(unsigned long catch_code, unsigned long exception_code)
{
    // TODO FIXME inheritance!
    return catch_code == exception_code ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}
