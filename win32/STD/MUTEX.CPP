#include <mutex>

#include <windows.h>

#include <common.h>


mutex::mutex()
{
    cs = new CRITICAL_SECTION();
    InitializeCriticalSection((CRITICAL_SECTION *)cs);
}

mutex::~mutex()
{
    DeleteCriticalSection((CRITICAL_SECTION *)cs);
    delete cs;
}

void mutex::lock()
{
    EnterCriticalSection((CRITICAL_SECTION *)cs);
}

void mutex::unlock()
{
    LeaveCriticalSection((CRITICAL_SECTION *)cs);
}
