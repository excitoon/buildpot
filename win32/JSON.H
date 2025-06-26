#ifndef JSON_DEFINED
#define JSON_DEFINED

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <stddef.h>

#include <common.h>


class JSONVector;
class StringJSONPairVector;

enum JSONType
{
    JSONNull,
    JSONBoolean,
    JSONNumber,
    JSONString,
    JSONArray,
    JSONObject,
    JSONError, // TODO replace with JSONUniquePtr
};

class JSON
{
    JSONType value_type;
    union Value
    {
        int boolean;
        string * string_or_number;
        JSONVector * array;
        StringJSONPairVector * object;
    } value;

    static JSON parse(const string & json_string, size_t & pos);

public:
    JSON();
    JSON(bool value);
    JSON(int value);
    JSON(long value);
    JSON(unsigned int value);
    JSON(unsigned long value);
    JSON(double value);
    JSON(const string & value);
    JSON(const JSONVector & value);
    JSON(const StringJSONPairVector & value);
    JSON(const JSON & rhs);
    ~JSON();

    JSON & operator =(const JSON & rhs);

    JSONType type() const;

    bool is_null() const;
    bool bool_value() const;
    const string & string_value() const;

    size_t size() const;

    const JSON & operator [](size_t index) const;
    JSON & operator [](size_t index);

    const JSON & operator [](const string & key) const;
    JSON & operator [](const string & key);

    const StringJSONPairVector & items() const;

    void push_back(const JSON & value);
    void push_back(const string & key, const JSON & value);

    static JSON from_string(const string & json_string);
    StringSharedPtr to_string() const;

    static const JSON & error(); // TODO replace with JSONUniquePtr
};

#endif // JSON_DEFINED
