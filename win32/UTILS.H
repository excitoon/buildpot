#ifndef UTILS_DEFINED
#define UTILS_DEFINED

#include <string>
#include <utility>
#include <vector>

#include <common.h>


class StringTriplet
{
public:
    string first;
    string second;
    string third;

    StringTriplet(const string & first, const string & second, const string & third);
};

string lower(const string & str);
string upper(const string & str);
string capitalize(const string & str);

bool isupper(const string & str);
bool islower(const string & str);

string hex(const string & str);
StringBoolPair fromhex(const string & str);

bool endswith(const string & str, const string & suffix);
bool startswith(const string & str, const string & prefix);

bool exists(const string & path);
bool isfile(const string & path);
bool isdir(const string & path);

string abspath(const string & path);

StringVector listdir(const string & path);

bool rmtree(const string & path);
bool makedirs(const string & path, bool exist_ok = false);

string path_join(const StringVector & parts);
string path_join(const string & first, const string & second);
string path_join(const string & first, const string & second, const string & third);
string path_join(const string & first, const string & second, const string & third, const string & fourth);
string path_join(const string & first, const string & second, const string & third, const string & fourth, const string & fifth);
StringVector path_split(const string & path);
StringPair path_splitext(const string & path);

string dirname(const string & path);
string basename(const string & path);

StringVector split(const string & str, const string & sep);
string join(const string & sep, const StringVector & parts);

StringTriplet partition(const string & str, const string & sep);
StringTriplet rpartition(const string & str, const string & sep);

string replace(const string & str, const string & old_value, const string & new_value, int count = -1);

#endif // UTILS_DEFINED
