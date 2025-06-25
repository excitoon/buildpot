#include <fstream>

#include <string>

#include <stdio.h>


const ios_base::openmode ios_base::app = 0x01;
const ios_base::openmode ios_base::ate = 0x02;
const ios_base::openmode ios_base::binary = 0x04;
const ios_base::openmode ios_base::in = 0x08;
const ios_base::openmode ios_base::out = 0x10;
const ios_base::openmode ios_base::trunc = 0x20;

const ios_base::iostate ios_base::goodbit = 0;
const ios_base::iostate ios_base::badbit = 1;
const ios_base::iostate ios_base::eofbit = 2;
const ios_base::iostate ios_base::failbit = 4;

static string convert_mode(ios_base::openmode mode)
{
    string fopen_mode;
    if (mode & ios_base::in && mode & ios_base::out)
    {
        fopen_mode = "r+";
    }
    else if (mode & ios_base::in)
    {
        fopen_mode = "r";
    }
    else if (mode & ios_base::out)
    {
        fopen_mode = "w";
    }
    if (mode & ios_base::binary)
    {
        fopen_mode += "b";
    }
    return fopen_mode;
}

fstream::fstream(const char * filename, ios_base::openmode mode)
{
    file = fopen(filename, convert_mode(mode).c_str());
    if (file == nullptr)
    {
        state = ios_base::badbit | ios_base::failbit;
    }
    else
    {
        state = ios_base::goodbit;
    }
}

fstream::fstream(const string & filename, ios_base::openmode mode)
{
    file = fopen(filename.c_str(), convert_mode(mode).c_str());
    if (file == nullptr)
    {
        state = ios_base::badbit | ios_base::failbit;
    }
    else
    {
        state = ios_base::goodbit;
    }
}

fstream::~fstream()
{
    if (file != nullptr)
    {
        fclose((FILE *)file);
    }
}

bool fstream::is_open() const
{
    return file != nullptr;
}

fstream::operator int() const
{
    return state == ios_base::goodbit;
}

void fstream::write(const char * data, size_t size)
{
    if (file == nullptr)
    {
        state = ios_base::badbit | ios_base::failbit;
        return;
    }
    if (fwrite(data, 1, size, (FILE *)file) != size)
    {
        state |= ios_base::failbit;
    }
}

void fstream::read(char * data, size_t size)
{
    if (file == nullptr)
    {
        state = ios_base::badbit | ios_base::failbit;
        return;
    }
    if (fread(data, 1, size, (FILE *)file) != size)
    {
        state |= ios_base::failbit;
    }
    if (feof((FILE *)file))
    {
        state |= ios_base::eofbit;
    }
}

void fstream::seekg(ssize_t offset)
{
    if (file == nullptr)
    {
        state = ios_base::badbit | ios_base::failbit;
        return;
    }
    if (fseek((FILE *)file, offset, SEEK_SET) != 0)
    {
        state |= ios_base::failbit;
    }
}

void fstream::seekg(ssize_t offset, ios_base::seekdir dir)
{
    if (file == nullptr)
    {
        state = ios_base::badbit | ios_base::failbit;
        return;
    }
    int whence;
    switch (dir)
    {
        case ios_base::beg:
            whence = SEEK_SET;
            break;
        case ios_base::cur:
            whence = SEEK_CUR;
            break;
        case ios_base::end:
            whence = SEEK_END;
            break;
        default:
            state |= ios_base::failbit;
            return;
    }
    if (fseek((FILE *)file, offset, whence) != 0)
    {
        state |= ios_base::failbit;
    }
}

void fstream::seekp(ssize_t offset)
{
    seekg(offset);
}

void fstream::seekp(ssize_t offset, ios_base::seekdir dir)
{
    seekg(offset, dir);
}

ssize_t fstream::tellg()
{
    if (file == nullptr)
    {
        state = ios_base::badbit | ios_base::failbit;
        return 0;
    }
    long pos = ftell((FILE *)file);
    if (pos == -1)
    {
        state |= ios_base::failbit;
    }
    return pos;
}

ssize_t fstream::tellp()
{
    return tellg();
}
