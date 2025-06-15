#ifndef JSON_DEFINED
#define JSON_DEFINED

#include <string>

#include <stddef.h>

#include <common.h>


enum JSONType
{
    JSONNull,
    JSONBoolean,
    JSONNumber,
    JSONString,
    JSONArray,
    JSONObject,
    JSONError,
};

class JSON
{
    JSONType value_type;
    union Value
    {
        bool boolean;
        string * string_or_number;
        struct {
            JSON ** elements;
            size_t size;
        } array;
        struct
        {
            string ** keys;
            JSON ** values;
            size_t size;
        } object;
    } value;

    static JSON parse(const string & json_string, size_t & pos);

public:
    JSON();
    JSON(bool value);
    JSON(long value);
    JSON(unsigned long value);
    JSON(double value);
    JSON(const string & value);
    JSON(size_t size, JSON ** elements); // Move semantics.
    JSON(size_t size, string ** keys, JSON ** values); // Move semantics.
    JSON(const JSON & rhs);
    ~JSON();

    JSON & operator =(const JSON & rhs);

    JSONType type() const;

    bool is_null() const;
    bool bool_value() const;
    const string & string_value() const;
    size_t size() const;
    const JSON & operator [](size_t index) const;
    const JSON & operator [](const string & key) const;
    JSON & operator [](const string & key);

    void push_back(const JSON & value);
    void push_back(const string & key, const JSON & value);

    static JSON from_string(const string & json_string);
    string to_string() const;

    static JSON error();
};

#endif
