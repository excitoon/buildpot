#include <utils.h>

#include <string>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <common.h>


StringWithError::StringWithError()
    : is_error(false)
{
}

StringWithError & StringWithError::operator =(const string & rhs)
{
    data = rhs;
    return *this;
}

string & StringWithError::operator *()
{
    return data;
}

const string & StringWithError::operator *() const
{
    return data;
}

string * StringWithError::operator ->()
{
    return &data;
}

const string * StringWithError::operator ->() const
{
    return &data;
}

StringWithError::operator bool() const
{
    return !is_error;
}

StringVector::StringVector()
    : data(nullptr)
    , length(0)
    , capacity(0)
{
}

StringVector::StringVector(const StringVector & rhs)
    : data(nullptr)
    , length(rhs.length)
    , capacity(rhs.capacity)
{
    if (capacity > 0)
    {
        data = new string[capacity];
        for (size_t i = 0; i < length; ++i)
        {
            data[i] = rhs.data[i];
        }
    }
}

StringVector::~StringVector()
{
    if (data != nullptr)
    {
        delete[] data;
    }
}

StringVector & StringVector::operator =(const StringVector & rhs)
{
    if (this != &rhs)
    {
        if (data != nullptr)
        {
            delete[] data;
        }
        length = rhs.length;
        capacity = rhs.capacity;
        if (capacity > 0)
        {
            data = new string[capacity];
            for (size_t i = 0; i < length; ++i)
            {
                data[i] = rhs.data[i];
            }
        }
        else
        {
            data = nullptr;
        }
    }
    return *this;
}

void StringVector::push_back(const string & str)
{
    if (length >= capacity)
    {
        capacity = (capacity == 0) ? 1 : capacity * 2;
        string * new_data = new string[capacity];
        for (size_t i = 0; i < length; ++i)
        {
            new_data[i] = data[i];
        }
        delete[] data;
        data = new_data;
    }
    data[length++] = str;
}

void StringVector::clear()
{
    if (data != nullptr)
    {
        delete[] data;
        data = nullptr;
    }
    length = 0;
    capacity = 0;
}

size_t StringVector::size() const
{
    return length;
}

const string & StringVector::operator [](size_t index) const
{
    return data[index];
}

string & StringVector::operator [](size_t index)
{
    return data[index];
}

StringPair::StringPair(const string & first, const string & second)
    : first(first)
    , second(second)
{
}

StringTriplet::StringTriplet(const string & first, const string & second, const string & third)
    : first(first)
    , second(second)
    , third(third)
{
}

string lower(const string & str)
{
    string result = str;
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = tolower(result[i]);
    }
    return result;
}

string upper(const string & str)
{
    string result = str;
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = toupper(result[i]);
    }
    return result;
}

string capitalize(const string & str)
{
    if (str.empty())
    {
        return str;
    }
    string result = str;
    result[0] = toupper(result[0]);
    for (size_t i = 1; i < result.size(); ++i)
    {
        result[i] = tolower(result[i]);
    }
    return result;
}

string hex(const string & str)
{
    string result;
    for (const char * it = &str.front(); it <= &str.back(); ++it)
    {
        char buf[3];
        _snprintf(buf, sizeof(buf), "%02x", (unsigned char)*it);
        result += buf;
    }
    return result;
}

StringWithError fromhex(const string & str)
{
    StringWithError result;
    if (str.size() % 2 != 0)
    {
        result.is_error = true;
        return result; // Invalid hex string.
    }
    result->reserve(str.size() / 2);
    for (size_t i = 0; i < str.size(); i += 2)
    {
        char buf[3] = { str[i], str[i + 1], '\0' };
        char * endptr;
        long value = strtol(buf, &endptr, 16);
        if (*endptr != '\0')
        {
            result.is_error = true;
            return result; // Invalid hex character.
        }
        result->push_back((char)value);
    }
    return result;
}

bool endswith(const string & str, const string & suffix)
{
    if (str.size() < suffix.size())
    {
        return false;
    }
    return str.substr(str.size() - suffix.size()) == suffix;
}

bool startswith(const string & str, const string & prefix)
{
    if (str.size() < prefix.size())
    {
        return false;
    }
    return str.substr(0, prefix.size()) == prefix;
}

bool exists(const string & path)
{
    FILE * fp = fopen(path.c_str(), "rb");
    if (fp != nullptr)
    {
        fclose(fp);
        return true;
    }
    return false;
}

StringPair splitext(const string & path)
{
    size_t pos = path.rfind('.');
    if (pos == string::npos)
    {
        return StringPair(path, "");
    }
    else
    {
        return StringPair(path.substr(0, pos), path.substr(pos));
    }
}

StringTriplet partition(const string & str, const string & sep)
{
    size_t pos = str.find(sep);
    if (pos == string::npos)
    {
        return StringTriplet(str, "", "");
    }
    else
    {
        return StringTriplet(str.substr(0, pos), sep, str.substr(pos + sep.size()));
    }
}

StringTriplet rpartition(const string & str, const string & sep)
{
    size_t pos = str.rfind(sep);
    if (pos == string::npos)
    {
        return StringTriplet("", "", str);
    }
    else
    {
        return StringTriplet(str.substr(0, pos), sep, str.substr(pos + sep.size()));
    }
}
