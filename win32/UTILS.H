#ifndef UTILS_DEFINED
#define UTILS_DEFINED

#include <string>

#include <common.h>


struct StringWithError
{
    string data;
    bool is_error;

    StringWithError & operator =(const string & rhs);
    const string & operator *() const;
    string & operator *();
    const string * operator ->() const;
    string * operator ->();
    operator bool() const;

    StringWithError();
};

class StringVector
{
    string * data;
    size_t length;
    size_t capacity;

public:
    StringVector();
    StringVector(const StringVector & rhs);
    ~StringVector();

    StringVector & operator =(const StringVector & rhs);

    void push_back(const string & str);
    void clear();
    size_t size() const;
    const string & operator [](size_t index) const;
    string & operator [](size_t index);
};

class StringPair
{
public:
    string first;
    string second;

    StringPair(const string & first, const string & second);
};

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

string hex(const string & str);
StringWithError fromhex(const string & str);

bool endswith(const string & str, const string & suffix);
bool startswith(const string & str, const string & prefix);

bool exists(const string & path);

StringPair splitext(const string & path);

StringTriplet partition(const string & str, const string & sep);
StringTriplet rpartition(const string & str, const string & sep);

#endif
