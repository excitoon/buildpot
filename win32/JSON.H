#ifndef JSON_DEFINED
#define JSON_DEFINED

#include <string>

#include <common.h>

#include <stddef.h>


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
    JSON(JSON ** elements, size_t size); // Move semantics.
    JSON(string ** keys, JSON ** values, size_t size); // Move semantics.
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

    static JSON from_string(const string & json_string);
    string to_string() const;

    static JSON error();
};

#endif
