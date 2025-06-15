#include <string>

#include <stddef.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>


const size_t string::npos = 0xffffffff;


string::string()
{
    buffer = new char[1]; // FIXME improve that.
    buffer[0] = '\0';
    reserved = length = 0;
}

string::string(const string & rhs)
{
    reserved = rhs.reserved;
    length = rhs.length;
    buffer = new char[reserved + 1];
    memcpy(buffer, rhs.buffer, length);
    buffer[length] = '\0';
}

string::string(const char * s, const char * e)
{
    reserved = length = e - s;
    buffer = new char[reserved + 1];
    memcpy(buffer, s, length);
    buffer[length] = '\0';
}

string::string(const char * s)
{
    reserved = length = strlen(s);
    buffer = new char[reserved+1];
    memcpy(buffer, s, length);
    buffer[length] = '\0';
}

string::string(const char * s, size_t len)
{
    reserved = length = len;
    buffer = new char[reserved+1];
    memcpy(buffer, s, length);
    buffer[length] = '\0';
}

void string::reserve(size_t n)
{
    if (n > reserved)
    {
        reserved = n;
        char * new_buffer = new char[reserved + 1];
        memcpy(new_buffer, buffer, length);
        delete[] buffer;
        buffer = new_buffer;
        buffer[length] = '\0';
    }
}

size_t string::find(const char * needle, size_t pos) const
{
    size_t needle_size = strlen(needle);
    for (size_t left = pos; left <= length - needle_size; ++left)
    {
        bool no_match = false;
        for (size_t i = left; i < left + needle_size; ++i)
        {
            if (buffer[i] != needle[i-left])
            {
                no_match = true;
                break;
            }
        }
        if (no_match)
        {
            continue;
        }
        return left;
    }
    return npos;
}

size_t string::find(char tip, size_t pos) const
{
    for (size_t left = pos; left < length; ++left)
    {
        if (buffer[left] == tip)
        {
            return left;
        }
    }
    return npos;
}

size_t string::find(const string & needle, size_t pos) const
{
    if (needle.length == 0)
    {
        return pos < length ? pos : npos;
    }
    size_t needle_size = needle.length;
    for (size_t left = pos; left <= length - needle_size; ++left)
    {
        bool no_match = false;
        for (size_t i = left; i < left + needle_size; ++i)
        {
            if (buffer[i] != needle.buffer[i-left])
            {
                no_match = true;
                break;
            }
        }
        if (no_match)
        {
            continue;
        }
        return left;
    }
    return npos;
}

size_t string::rfind(char tip, size_t pos) const
{
    if (pos >= length)
    {
        pos = length - 1;
    }
    for (size_t right = pos; right != npos; --right)
    {
        if (buffer[right] == tip)
        {
            return right;
        }
    }
    return npos;
}

size_t string::rfind(const string & needle, size_t pos) const
{
    if (needle.length == 0)
    {
        return pos < length ? pos : npos;
    }
    if (pos >= length)
    {
        pos = length - 1;
    }
    size_t needle_size = needle.length;
    for (size_t right = pos; right >= needle_size - 1; --right)
    {
        bool no_match = false;
        for (size_t i = right; i > right - needle_size; --i)
        {
            if (buffer[i] != needle.buffer[i - right + needle_size - 1])
            {
                no_match = true;
                break;
            }
        }
        if (no_match)
        {
            continue;
        }
        return right - needle_size + 1;
    }
    return npos;
}

string string::substr(size_t pos, size_t len) const
{
    if (pos >= length)
    {
        return string();
    }
    if (len == npos || pos + len > length)
    {
        len = length - pos;
    }
    string result;
    result.reserved = len;
    result.length = len;
    result.buffer = new char[len + 1];
    memcpy(result.buffer, buffer + pos, len);
    result.buffer[len] = '\0';
    return result;
}

string & string::operator =(const char * rhs)
{
    size_t rhs_length = strlen(rhs);
    if (rhs_length > reserved)
    {
        delete[] buffer;
        reserved = rhs_length;
        buffer = new char[reserved + 1];
    }
    length = rhs_length;
    memcpy(buffer, rhs, length);
    buffer[length] = '\0';
    return *this;
}

string & string::operator =(const string & rhs)
{
    if (this != &rhs)
    {
        delete[] buffer;
        length = rhs.length;
        reserved = rhs.reserved;
        buffer = new char[reserved + 1];
        memcpy(buffer, rhs.buffer, length);
        buffer[length] = '\0';
    }
    return *this;
}

string string::operator +(const string & rhs) const
{
    string result;
    result.length = result.reserved = length + rhs.length;
    result.buffer = new char[result.reserved + 1];
    memcpy(result.buffer, buffer, length);
    memcpy(result.buffer + length, rhs.buffer, rhs.length);
    result.buffer[result.length] = '\0';
    return result;
}

bool string::operator ==(const char * rhs) const
{
    size_t rhs_length = strlen(rhs);
    if (rhs_length != length)
    {
        return false;
    }
    else
    {
        for (size_t i = 0; i < length; ++i)
        {
            if (buffer[i] != rhs[i])
            {
                return false;
            }
        }
        return true;
    }
}

bool string::operator ==(const string & rhs) const
{
    if (rhs.length != length)
    {
        return false;
    }
    else
    {
        for (size_t i = 0; i < length; ++i)
        {
            if (buffer[i] != rhs.buffer[i])
            {
                return false;
            }
        }
        return true;
    }
}

bool string::operator !=(const char * rhs) const
{
    return !(*this == rhs);
}

bool string::operator !=(const string & rhs) const
{
    return !(*this == rhs);
}

string & string::operator +=(const char c)
{
    push_back(c);
    return *this;
}

string & string::operator +=(const string & rhs)
{
    if (rhs.length > 0)
    {
        if (length + rhs.length > reserved)
        {
            reserved = length + rhs.length;
            char * new_buffer = new char[reserved + 1];
            if (buffer)
            {
                memcpy(new_buffer, buffer, length);
                delete[] buffer;
            }
            buffer = new_buffer;
        }
        memcpy(buffer + length, rhs.buffer, rhs.length);
        length += rhs.length;
        buffer[length] = '\0';
    }
    return *this;
}

const char & string::operator [](size_t pos) const
{
    return buffer[pos];
}

char & string::operator [](size_t pos)
{
    return buffer[pos];
}

void string::erase(size_t pos, size_t len)
{
    if (pos < length)
    {
        if (pos + len > length)
        {
            len = length - pos;
        }
        memmove(buffer + pos, buffer + pos + len, length - pos - len);
        length -= len;
        buffer[length] = '\0';
    }
}

void string::clear()
{
    if (buffer != nullptr)
    {
        delete[] buffer;
        buffer = nullptr;
    }
    length = reserved = 0;
}

size_t string::find_first_of(const char * chars, size_t pos) const
{
    for (size_t i = pos; i < length; ++i)
    {
        if (strchr(chars, buffer[i]) != nullptr)
        {
            return i;
        }
    }
    return npos;
}

size_t string::find_first_not_of(const char * chars, size_t pos) const
{
    for (size_t i = pos; i < length; ++i)
    {
        if (strchr(chars, buffer[i]) == nullptr)
        {
            return i;
        }
    }
    return npos;
}

bool string::empty() const
{
    return length == 0;
}

size_t string::size() const
{
    return length;
}

char * string::data()
{
    return buffer ? buffer : "";
}

const char * string::data() const
{
    return buffer ? buffer : "";
}

const char * string::c_str() const
{
    return data();
}

void string::pop_back()
{
    if (length > 0)
    {
        --length;
        buffer[length] = '\0';
    }
}

const char & string::front() const
{
    return buffer[0];
}

char & string::front()
{
    return buffer[0];
}

char & string::back()
{
    return buffer[length - 1];
}

const char & string::back() const
{
    return buffer[length - 1];
}

void string::push_back(char c)
{
    reserve(length + 1);
    buffer[length] = c;
    ++length;
    buffer[length] = '\0';
}

string::~string()
{
    if (buffer != nullptr)
    {
        delete[] buffer;
    }
}

string operator +(const char * lhs, const string & rhs)
{
    string result(lhs);
    result += rhs;
    return result;
}

string to_string(int value)
{
    char buffer[32];
    _snprintf(buffer, sizeof(buffer)-1, "%d", value);
    return string(buffer);
}

string to_string(unsigned int value)
{
    char buffer[32];
    _snprintf(buffer, sizeof(buffer)-1, "%u", value);
    return string(buffer);
}

string to_string(long value)
{
    char buffer[32];
    _snprintf(buffer, sizeof(buffer)-1, "%ld", value);
    return string(buffer);
}

string to_string(unsigned long value)
{
    char buffer[32];
    _snprintf(buffer, sizeof(buffer)-1, "%lu", value);
    return string(buffer);
}

string to_string(double value)
{
    char buffer[32];
    _snprintf(buffer, sizeof(buffer)-1, "%lf", value);
    return string(buffer);
}
