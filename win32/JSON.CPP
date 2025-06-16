#include <json.h>

#include <string>

#include <logger.h>
#include <memory.h>
#include <stddef.h>
#include <stdio.h>


const string whitespace = " \t\n\r";
JSON json_error;

JSON::JSON()
    : value_type(JSONNull)
{
}
/*
JSON::JSON(bool boolean)
    : value_type(JSONBoolean)
{
    value.boolean = boolean;
}
*/
JSON::JSON(int number)
    : value_type(JSONNumber)
{
    value.string_or_number = new string(::to_string(number));
}

JSON::JSON(long number)
    : value_type(JSONNumber)
{
    value.string_or_number = new string(::to_string(number));
}

JSON::JSON(unsigned int number)
    : value_type(JSONNumber)
{
    value.string_or_number = new string(::to_string(number));
}

JSON::JSON(unsigned long number)
    : value_type(JSONNumber)
{
    value.string_or_number = new string(::to_string(number));
}

JSON::JSON(double number)
    : value_type(JSONNumber)
{
    value.string_or_number = new string(::to_string(number));
}

JSON::JSON(const string & string)
    : value_type(JSONString)
{
    value.string_or_number = new class string(string);
}

JSON::JSON(size_t size, JSON ** elements)
    : value_type(JSONArray)
{
    value.array.elements = elements;
    value.array.size = size;
}

JSON::JSON(size_t size, string ** keys, JSON ** values)
    : value_type(JSONObject)
{
    value.object.keys = keys;
    value.object.values = values;
    value.object.size = size;
}

JSON::JSON(const JSON & rhs)
    : value_type(rhs.value_type)
{
    switch (value_type)
    {
        case JSONNull:
        case JSONBoolean:
            value.boolean = rhs.value.boolean;
            break;

        case JSONString:
        case JSONNumber:
            value.string_or_number = new string(*rhs.value.string_or_number);
            break;

        case JSONArray:
            value.array.size = rhs.value.array.size;
            if (value.array.size > 0)
            {
                value.array.elements = new JSON *[value.array.size];
                for (size_t i = 0; i < value.array.size; ++i)
                {
                    value.array.elements[i] = new JSON(*rhs.value.array.elements[i]);
                }
            }
            break;

        case JSONObject:
            value.object.size = rhs.value.object.size;
            if (value.object.size > 0)
            {
                value.object.keys = new string *[value.object.size];
                value.object.values = new JSON *[value.object.size];
                for (size_t i = 0; i < value.object.size; ++i)
                {
                    value.object.keys[i] = new string(*rhs.value.object.keys[i]);
                    value.object.values[i] = new JSON(*rhs.value.object.values[i]);
                }
            }
            break;
    }
}

JSON::~JSON()
{
    switch (value_type)
    {
        case JSONString:
        case JSONNumber:
        {
            delete value.string_or_number;
            break;
        }

        case JSONArray:
        {
            for (size_t i = 0; i < value.array.size; ++i)
            {
                delete value.array.elements[i];
            }
            if (value.array.size > 0)
            {
                delete[] value.array.elements;
            }
            break;
        }

        case JSONObject:
        {
            for (size_t i = 0; i < value.object.size; ++i)
            {
                delete value.object.keys[i];
                delete value.object.values[i];
            }
            if (value.object.size > 0)
            {
                delete[] value.object.keys;
                delete[] value.object.values;
            }
            break;
        }
    }
}

JSON & JSON::operator =(const JSON & rhs)
{
    if (this != &rhs)
    {
        this->~JSON();
        this->JSON::JSON(rhs);
    }
    return *this;
}

JSONType JSON::type() const
{
    return value_type;
}

bool JSON::is_null() const
{
    return value_type == JSONNull;
}

bool JSON::bool_value() const
{
    return value.boolean;
}

const string & JSON::string_value() const
{
    return *value.string_or_number;
}

size_t JSON::size() const
{
    switch (value_type)
    {
        case JSONArray:
            return value.array.size;

        case JSONObject:
            return value.object.size;

        default:
            return 0;
    }
}

const JSON & JSON::operator [](size_t index) const
{
    return *value.array.elements[index];
}

JSON & JSON::operator [](size_t index)
{
    return *value.array.elements[index];
}

const JSON & JSON::operator [](const string & key) const
{
    for (size_t i = 0; i < value.object.size; ++i)
    {
        if (*value.object.keys[i] == key)
        {
            return *value.object.values[i];
        }
    }
    return JSON::error();
}

JSON & JSON::operator [](const string & key)
{
    for (size_t i = 0; i < value.object.size; ++i)
    {
        if (*value.object.keys[i] == key)
        {
            return *value.object.values[i];
        }
    }
    push_back(key, JSON());
    return *value.object.values[value.object.size - 1];
}

JSON JSON::keys() const
{
    JSON ** keys_array = new JSON *[value.object.size];
    for (size_t i = 0; i < value.object.size; ++i)
    {
        keys_array[i] = new JSON(*value.object.keys[i]);
    }
    return JSON(value.object.size, keys_array);
}

JSON JSON::values() const
{
    JSON ** values_array = new JSON *[value.object.size];
    for (size_t i = 0; i < value.object.size; ++i)
    {
        values_array[i] = new JSON(*value.object.values[i]);
    }
    return JSON(value.object.size, values_array);
}

void JSON::push_back(const JSON & element)
{
    JSON ** new_elements = new JSON *[value.array.size + 1];
    memcpy(new_elements, value.array.elements, sizeof(JSON *) * value.array.size);
    if (value.array.size > 0)
    {
        delete[] value.array.elements;
    }
    new_elements[value.array.size] = new JSON(element);
    value.array.elements = new_elements;
    ++value.array.size;
}

void JSON::push_back(const string & key, const JSON & element_value)
{
    string ** new_keys = new string *[value.object.size + 1];
    JSON ** new_values = new JSON *[value.object.size + 1];
    memcpy(new_keys, value.object.keys, sizeof(string *) * value.object.size);
    memcpy(new_values, value.object.values, sizeof(JSON *) * value.object.size);
    if (value.object.size > 0)
    {
        delete[] value.object.keys;
        delete[] value.object.values;
    }
    new_keys[value.object.size] = new string(key);
    new_values[value.object.size] = new JSON(element_value);
    value.object.keys = new_keys;
    value.object.values = new_values;
    ++value.object.size;
}

JSON JSON::from_string(const string & json_string)
{
    size_t pos = 0;
    JSON result = parse(json_string, pos);
    while (pos < json_string.size() && whitespace.find(json_string[pos]) != string::npos)
    {
        ++pos;
    }
    if (pos < json_string.size())
    {
        return JSON::error();
    }
    else
    {
        return result;
    }
}

string JSON::to_string() const
{
    switch (value_type)
    {
        case JSONNull:
            return "null";

        case JSONBoolean:
            return value.boolean ? "true" : "false";

        case JSONNumber:
            return *value.string_or_number;

        case JSONString:
        {
            string escaped_and_quoted = "\"";
            for (const char * c = &value.string_or_number->front(); c <= &value.string_or_number->back(); ++c)
            {
                switch (*c)
                {
                    case '\"': escaped_and_quoted += "\\\""; break;
                    case '\\': escaped_and_quoted += "\\\\"; break;
                    case '\b': escaped_and_quoted += "\\b"; break;
                    case '\f': escaped_and_quoted += "\\f"; break;
                    case '\n': escaped_and_quoted += "\\n"; break;
                    case '\r': escaped_and_quoted += "\\r"; break;
                    case '\t': escaped_and_quoted += "\\t"; break;
                    default:
                        if ((unsigned char)(*c) < 0x20)
                        {
                            char buf[7];
                            _snprintf(buf, sizeof(buf), "\\u00%02x", (unsigned char)(*c));
                            escaped_and_quoted += buf;
                        }
                        else
                        {
                            escaped_and_quoted += *c;
                        }
                }
            }
            escaped_and_quoted += "\"";
            return escaped_and_quoted;
        }

        case JSONArray:
        {
            string result = "[";
            for (size_t i = 0; i < value.array.size; ++i)
            {
                if (i > 0)
                {
                    result += ", ";
                }
                result += value.array.elements[i]->to_string();
            }
            result += "]";
            return result;
        }

        case JSONObject:
        {
            string result = "{";
            for (size_t i = 0; i < value.object.size; ++i)
            {
                if (i > 0)
                {
                    result += ", ";
                }
                result += "\"" + *value.object.keys[i] + "\": " + value.object.values[i]->to_string();
            }
            result += "}";
            return result;
        }

        default:
            return "";
    }
}

const JSON & JSON::error()
{
    json_error.value_type = JSONError;
    return json_error;
}

JSON JSON::parse(const string & json_string, size_t & pos)
{
    const char * it = json_string.data() + pos;
    while (it <= &json_string.back() && whitespace.find(*it) != string::npos)
    {
        ++it;
    }
    if (it > &json_string.back())
    {
        pos = json_string.size();
        return JSON::error();
    }
    switch (*it)
    {
        case '{':
        {

            ++it;
            JSON result = JSON(0, nullptr, nullptr);
            while (true)
            {
                while (it <= &json_string.back() && whitespace.find(*it) != string::npos)
                {
                    ++it;
                }
                if (it > &json_string.back())
                {
                    pos = json_string.size();
                    break;
                }
                if (*it == '}')
                {
                    ++it;
                    pos = it - json_string.data();
                    return result;
                }
                pos = it - json_string.data();
                JSON key = parse(json_string, pos);
                if (key.type() == JSONError)
                {
                    break;
                }
                else if (key.type() != JSONString)
                {
                    pos = it - json_string.data();
                    break;
                }
                it = json_string.data() + pos;
                while (it <= &json_string.back() && whitespace.find(*it) != string::npos)
                {
                    ++it;
                }
                if (it > &json_string.back())
                {
                    pos = json_string.size();
                    break;
                }
                if (*it != ':')
                {
                    pos = it - json_string.data();
                    break;
                }
                ++it;
                while (it <= &json_string.back() && whitespace.find(*it) != string::npos)
                {
                    ++it;
                }
                if (it > &json_string.back())
                {
                    pos = json_string.size();
                    break;
                }
                pos = it - json_string.data();
                JSON value = parse(json_string, pos);
                if (value.type() == JSONError)
                {
                    break;
                }
                result.push_back(key.string_value(), value);
                it = json_string.data() + pos;
                while (it <= &json_string.back() && whitespace.find(*it) != string::npos)
                {
                    ++it;
                }
                if (it > &json_string.back())
                {
                    pos = json_string.size();
                    break;
                }
                if (*it == ',')
                {
                    ++it;
                }
                else if (*it != '}')
                {
                    pos = it - json_string.data();
                    break;
                }
            }
            return JSON::error();
        }
        case '[':
        {
            ++it;
            JSON result(0, nullptr);
            while (true)
            {
                while (it <= &json_string.back() && whitespace.find(*it) != string::npos)
                {
                    ++it;
                }
                if (it > &json_string.back())
                {
                    pos = json_string.size();
                    break;
                }
                if (*it == ']')
                {
                    ++it;
                    pos = it - json_string.data();
                    return result;
                }
                pos = it - json_string.data();
                JSON element = parse(json_string, pos);
                if (element.type() == JSONError)
                {
                    break;
                }
                result.push_back(element);
                it = json_string.data() + pos;
                while (it <= &json_string.back() && whitespace.find(*it) != string::npos)
                {
                    ++it;
                }
                if (it > &json_string.back())
                {
                    pos = json_string.size();
                    break;
                }
                if (*it == ',')
                {
                    ++it;
                }
                else if (*it != ']')
                {
                    pos = it - json_string.data();
                    break;
                }
            }
            return JSON::error();
        }
        case '\"':
        {
            ++it;
            string result;
            while (it <= &json_string.back())
            {
                switch (*it)
                {
                    case '\"':
                        ++it;
                        pos = it - json_string.data();
                        return JSON(result);
                    case '\\':
                        pos = it - json_string.data();
                        ++it;
                        if (it > &json_string.back())
                        {
                            pos = json_string.size();
                            return JSON::error();
                        }
                        switch (*it)
                        {
                            case '\"': result += '\"'; break;
                            case '\\': result += '\\'; break;
                            case '/': result += '/'; break;
                            case 'b': result += '\b'; break;
                            case 'f': result += '\f'; break;
                            case 'n': result += '\n'; break;
                            case 'r': result += '\r'; break;
                            case 't': result += '\t'; break;
                            case 'u':
                            {
                                unsigned int codepoint = 0;
                                for (size_t i = 0; i < 4; ++i)
                                {
                                    ++it;
                                    if (it > &json_string.back())
                                    {
                                        pos = it - json_string.data();
                                        return JSON::error();
                                    }
                                    else if (*it >= '0' && *it <= '9')
                                    {
                                        codepoint = (codepoint << 4) | (*it - '0');
                                    }
                                    else if (*it >= 'a' && *it <= 'f')
                                    {
                                        codepoint = (codepoint << 4) | (*it - 'a' + 10);
                                    }
                                    else if (*it >= 'A' && *it <= 'F')
                                    {
                                        codepoint = (codepoint << 4) | (*it - 'A' + 10);
                                    }
                                    else
                                    {
                                        pos = it - json_string.data();
                                        return JSON::error();
                                    }
                                }
                                if (codepoint >= 0x80)
                                {
                                    // TODO It would be neccessary to support both surrogate pairs and UTF-8.
                                    return JSON::error();
                                }
                                result += (char)codepoint;
                                break;
                            }
                            default:
                                pos = it - json_string.data();
                                return JSON::error();
                        }
                        break;
                    default:
                        result += *it;
                        break;
                }
                ++it;
            }
            pos = json_string.size();
            return JSON::error();
        }
        case 'n':
        {
            if (it + 3 > &json_string.back() || memcmp(it, "null", 4) != 0)
            {
                pos = it - json_string.data();
                return JSON::error();
            }
            it += 4;
            pos = it - json_string.data();
            return JSON();
        }
        case 't':
        {
            if (it + 3 > &json_string.back() || memcmp(it, "true", 4) != 0)
            {
                pos = it - json_string.data();
                return JSON::error();
            }
            it += 4;
            pos = it - json_string.data();
            return JSON(true);
        }
        case 'f':
        {
            if (it + 4 > &json_string.back() || memcmp(it, "false", 5) != 0)
            {
                pos = it - json_string.data();
                return JSON::error();
            }
            it += 5;
            pos = it - json_string.data();
            return JSON(false);
        }
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
        case '8': case '9': case '-':
        {
            // Numeric values that cannot be represented in the grammar below (such
            // as Infinity and NaN) are not permitted.

            //     number = [ minus ] int [ frac ] [ exp ]
            //     decimal-point = %x2E       ; .
            //     digit1-9 = %x31-39         ; 1-9
            //     e = %x65 / %x45            ; e E
            //     exp = e [ minus / plus ] 1*DIGIT
            //     frac = decimal-point 1*DIGIT
            //     int = zero / ( digit1-9 *DIGIT )
            //     minus = %x2D               ; -
            //     plus = %x2B                ; +
            //     zero = %x30                ; 0

            size_t sign_pos = it - json_string.data();
            if (*it == '-')
            {
                ++it;
            }
            size_t integer_pos = it - json_string.data();
            if (it > &json_string.back() || (*it < '0' || *it > '9'))
            {
                // End of string or not a digit.
                pos = it - json_string.data();
                return JSON::error();
            }
            ++it;
            if (it <= &json_string.back() && *(it-1) == '0' && *it >= '1' && *it <= '9')
            {
                // Zero padding is not allowed.
                pos = it + 1 - json_string.data();
                return JSON::error();
            }
            while (it <= &json_string.back() && *it >= '1' && *it <= '9')
            {
                ++it;
            }
            if (it > &json_string.back())
            {
                // It is the end of the string exactly.
                JSON integer = JSON();
                integer.value_type = JSONNumber;
                integer.value.string_or_number = new string(json_string.data() + sign_pos, it);
                pos = it - json_string.data();
                return integer;
            }
            // We have few options here:
            // - `.` means there is a fractional part;
            // - `e` or `E` means there is no fractional part, but there is an exponent;
            // - something else means it is the end of the number.
            size_t frac_pos = it - json_string.data();
            if (*it == '.')
            {
                ++it;
                if (it > &json_string.back() || (*it < '0' || *it > '9'))
                {
                    // End of string or not a digit.
                    pos = it - json_string.data();
                    return JSON::error();
                }
                ++it;
                while (it <= &json_string.back() && *it >= '0' && *it <= '9')
                {
                    ++it;
                }
            }
            size_t exp_pos = it - json_string.data();
            if (it > &json_string.back() || *it != 'e' && *it != 'E')
            {
                // We have our number.
                JSON number = JSON();
                number.value_type = JSONNumber;
                number.value.string_or_number = new string(json_string.data() + sign_pos, it);
                pos = it - json_string.data();
                return number;
            }
            ++it;
            if (it > &json_string.back())
            {
                pos = it - json_string.data();
                return JSON::error();
            }
            if (*it == '-' || *it == '+')
            {
                ++it;
            }
            if (it > &json_string.back() || *it < '0' || *it > '9')
            {
                // End of string or not a digit.
                pos = it - json_string.data();
                return JSON::error();
            }
            ++it;
            while (it <= &json_string.back() && *it >= '0' && *it <= '9')
            {
                ++it;
            }
            JSON number = JSON();
            number.value_type = JSONNumber;
            number.value.string_or_number = new string(json_string.data() + sign_pos, it);
            pos = it - json_string.data();
            return number;
        }
        default:
            pos = it - json_string.data();
            return JSON::error();
    }
}
