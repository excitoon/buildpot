#include <unistd.h>

#include <windows.h>


unsigned int sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}

int usleep(unsigned long useconds)
{
    Sleep(useconds / 1000);
    return 0;
}
