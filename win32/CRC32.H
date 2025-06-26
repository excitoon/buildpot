#ifndef CRC32_DEFINED
#define CRC32_DEFINED

#include <string>

#include <common.h>


class CRC32
{
    unsigned long crc;
    static const unsigned long table[256];

public:
    CRC32();
    void update(const string & data);
    unsigned long finalize();

private:
};

unsigned long crc32(const string & data);

#endif // CRC32_DEFINED
