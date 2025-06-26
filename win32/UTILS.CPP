#include <utils.h>

#include <cctype>
#include <string>
#include <utility>
#include <vector>

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#include <common.h>
#include <logger.h>


const unsigned long INVALID_FILE_ATTRIBUTES = 0xffffffff;

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

bool isupper(const string & str)
{
    bool has_cased = false;
    for (const char * it = &str.front(); it <= &str.back(); ++it)
    {
        if (isupper(*it))
        {
            has_cased = true;
        }
        else if (islower(*it))
        {
            return false;
        }
    }
    return has_cased;
}

bool islower(const string & str)
{
    bool has_cased = false;
    for (const char * it = &str.front(); it <= &str.back(); ++it)
    {
        if (islower(*it))
        {
            has_cased = true;
        }
        else if (isupper(*it))
        {
            return false;
        }
    }
    return has_cased;
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

StringBoolPair fromhex(const string & str)
{
    StringBoolPair result;
    if (str.size() % 2 != 0)
    {
        result.second = true;
        return result; // Invalid hex string.
    }
    result.first.reserve(str.size() / 2);
    for (size_t i = 0; i < str.size(); i += 2)
    {
        char buf[3] = { str[i], str[i + 1], '\0' };
        char * endptr;
        long value = strtol(buf, &endptr, 16);
        if (*endptr != '\0')
        {
            result.second = true;
            return result; // Invalid hex character.
        }
        result.first.push_back((char)value);
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
    return GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool isfile(const string & path)
{
    DWORD attributes = GetFileAttributes(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool isdir(const string & path)
{
    DWORD attributes = GetFileAttributes(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

string abspath(const string & path)
{
    char full_path[1024];
    unsigned long bytes = GetFullPathName(path.c_str(), sizeof(full_path), full_path, nullptr);
    if (bytes == 0 || bytes >= sizeof(full_path))
    {
        return string();
    }
    else
    {
        return full_path;
    }
}

StringVector listdir(const string & path)
{
    StringVector result;
    WIN32_FIND_DATA find_data;
    HANDLE hFind = FindFirstFile((path + "\\*.*").c_str(), &find_data);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return result;
    }
    do
    {
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0)
            {
                result.push_back(find_data.cFileName);
            }
        }
        else
        {
            result.push_back(find_data.cFileName);
        }
    } while (FindNextFile(hFind, &find_data));

    FindClose(hFind);
    return result;
}

bool rmtree(const string & path)
{
    StringVector names = listdir(path);
    for (size_t i = 0; i < names.size(); ++i)
    {
        string child_path = path_join(path, names[i]);
        if (isfile(child_path))
        {
            if (remove(child_path.c_str()) != 0)
            {
                Log(Utils, Error, "Failed to remove file `%s`. GetLastError() -> %d.", child_path.c_str(), GetLastError());
                return false;
            }
        }
        else if (isdir(child_path))
        {
            if (!rmtree(child_path))
            {
                Log(Utils, Error, "Failed to remove subdirectory `%s`. GetLastError() -> %d.", child_path.c_str(), GetLastError());
                return false;
            }
        }
        else
        {
            Log(Utils, Error, "Failed to remove `%s` as it is neither a file nor a directory.", child_path.c_str());
            return false;
        }
    }

    return RemoveDirectory(path.c_str());
}

bool makedirs(const string & path, bool exist_ok)
{
    string full_path = abspath(path);
    if (full_path.empty())
    {
        return false;
    }
    StringVector parts = path_split(full_path);
    StringVector new_parts;
    bool already_exists;
    while (new_parts.size() < parts.size())
    {
        new_parts.push_back(parts[new_parts.size()]);
        string current_path = path_join(new_parts);
        if (isfile(current_path))
        {
            return false;
        }
        already_exists = isdir(current_path);
        if (!already_exists)
        {
            if (!CreateDirectory(current_path.c_str(), nullptr))
            {
                return false;
            }
        }
    }
    return exist_ok || !already_exists;
}

string path_join(const StringVector & parts)
{
    StringVector non_empty_parts;
    for (size_t i = 0; i < parts.size(); ++i)
        if (!parts[i].empty())
            non_empty_parts.push_back(parts[i]);
    return join("\\", non_empty_parts);
}

string path_join(const string & first, const string & second)
{
    StringVector parts;
    parts.push_back(first);
    parts.push_back(second);
    return path_join(parts);
}

string path_join(const string & first, const string & second, const string & third)
{
    StringVector parts;
    parts.push_back(first);
    parts.push_back(second);
    parts.push_back(third);
    return path_join(parts);
}

string path_join(const string & first, const string & second, const string & third, const string & fourth)
{
    StringVector parts;
    parts.push_back(first);
    parts.push_back(second);
    parts.push_back(third);
    parts.push_back(fourth);
    return path_join(parts);
}

string path_join(const string & first, const string & second, const string & third, const string & fourth, const string & fifth)
{
    StringVector parts;
    parts.push_back(first);
    parts.push_back(second);
    parts.push_back(third);
    parts.push_back(fourth);
    parts.push_back(fifth);
    return path_join(parts);
}

StringVector path_split(const string & path)
{
    return split(path, "\\");
}

StringPair path_splitext(const string & path)
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

string dirname(const string & path)
{
    size_t pos = path.rfind('\\');
    if (pos == string::npos)
    {
        return "";
    }
    return path.substr(0, pos);
}

string basename(const string & path)
{
    size_t pos = path.rfind('\\');
    if (pos == string::npos)
    {
        return path;
    }
    return path.substr(pos + 1);
}

StringVector split(const string & str, const string & sep)
{
    StringVector result;
    size_t pos = 0;
    while (true)
    {
        size_t next_pos = str.find(sep, pos);
        if (next_pos == string::npos)
        {
            result.push_back(str.substr(pos));
            break;
        }
        else
        {
            result.push_back(str.substr(pos, next_pos - pos));
            pos = next_pos + sep.size();
        }
    }
    return result;
}

string join(const string & sep, const StringVector & parts)
{
    string result;
    if (parts.size() == 0)
    {
        return result;
    }
    size_t total_length = sep.size() * (parts.size() - 1);
    for (size_t i = 0; i < parts.size(); ++i)
    {
        total_length += parts[i].size();
    }
    result.reserve(total_length);
    for (i = 0; i < parts.size(); ++i)
    {
        if (i)
        {
            result += sep;
        }
        result += parts[i];
    }
    return result;
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

string replace(const string & str, const string & old_value, const string & new_value, int count)
{
    string result;
    size_t pos = 0;
    size_t old_value_length = old_value.size();
    size_t new_value_length = new_value.size();
    int replacements = 0;

    while (true)
    {
        size_t next_pos = str.find(old_value, pos);
        if (next_pos == string::npos || (count >= 0 && replacements >= count))
        {
            result += str.substr(pos);
            break;
        }
        else
        {
            result += str.substr(pos, next_pos - pos);
            result += new_value;
            pos = next_pos + old_value_length;
            ++replacements;
        }
    }
    return result;
}
