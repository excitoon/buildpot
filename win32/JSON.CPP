#include <json.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <memory.h>
#include <stddef.h>
#include <stdio.h>

#include <common.h>
#include <logger.h>
#include <utils.h>


const string whitespace = " \t\n\r";
JSON json_error;

JSON::JSON()
    : value_type(JSONNull)
{
}

JSON::JSON(bool boolean)
    : value_type(JSONBoolean)
{
    value.boolean = boolean;
}

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

JSON::JSON(const string & string_value)
    : value_type(JSONString)
{
    value.string_or_number = new string(string_value);
}

JSON::JSON(const JSONVector & array)
    : value_type(JSONArray)
{
    value.array = new JSONVector(array);
}

JSON::JSON(const StringJSONPairVector & object)
    : value_type(JSONObject)
{
    value.object = new StringJSONPairVector(object);
}

JSON::JSON(const JSON & rhs)
    : value_type(rhs.value_type)
{
    switch (value_type)
    {
        case JSONNull:
            break;

        case JSONBoolean:
            value.boolean = rhs.value.boolean;
            break;

        case JSONString:
        case JSONNumber:
            value.string_or_number = new string(*rhs.value.string_or_number);
            break;

        case JSONArray:
            value.array = new JSONVector(*rhs.value.array);
            break;

        case JSONObject:
            value.object = new StringJSONPairVector(*rhs.value.object);
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
            delete value.array;
            break;
        }

        case JSONObject:
        {
            delete value.object;
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
            return value.array->size();

        case JSONObject:
            return value.object->size();

        default:
            // TODO exception
            return 0;
    }
}

const JSON & JSON::operator [](size_t index) const
{
    return (*value.array)[index];
}

JSON & JSON::operator [](size_t index)
{
    return (*value.array)[index];
}

const JSON & JSON::operator [](const string & key) const
{
    for (size_t i = 0; i < value.object->size(); ++i)
    {
        if ((*value.object)[i].first == key)
        {
            return (*value.object)[i].second;
        }
    }
    // TODO exception
    return JSON::error();
}

JSON & JSON::operator [](const string & key)
{
    for (size_t i = 0; i < value.object->size(); ++i)
    {
        if ((*value.object)[i].first == key)
        {
            return (*value.object)[i].second;
        }
    }
    (*value.object).push_back(make_pair(key, JSON()));
    return value.object->back().second;
}

const StringJSONPairVector & JSON::items() const
{
    return *value.object;
}

void JSON::push_back(const JSON & element)
{
    value.array->push_back(element);
}

void JSON::push_back(const string & key, const JSON & element_value)
{
    value.object->push_back(make_pair(key, element_value));
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

static StringSharedPtr to_json_string(const string & value)
{
    string escaped_and_encoded = "\"";
    for (const char * c = &value.front(); c <= &value.back(); ++c)
    {
        switch (*c)
        {
            case '"': escaped_and_encoded += "\\\""; break;
            case '\\': escaped_and_encoded += "\\\\"; break;
            case '\b': escaped_and_encoded += "\\b"; break;
            case '\f': escaped_and_encoded += "\\f"; break;
            case '\n': escaped_and_encoded += "\\n"; break;
            case '\r': escaped_and_encoded += "\\r"; break;
            case '\t': escaped_and_encoded += "\\t"; break;
            default:
                unsigned int codepoint = (unsigned char)*c;
                if (codepoint >= 0x80)
                {
                    if ((codepoint & 0xe0) == 0xc0 && c + 1 <= &value.back() && (((unsigned char)*(c + 1)) & 0xc0) == 0x80)
                    {
                        codepoint = ((codepoint & 0x1f) << 6) | (((unsigned char)*(c + 1)) & 0x3f);
                        ++c;
                    }
                    else if ((codepoint & 0xf0) == 0xe0 && c + 2 <= &value.back() && (((unsigned char)*(c + 1)) & 0xc0) == 0x80 && (((unsigned char)*(c + 2)) & 0xc0) == 0x80)
                    {
                        codepoint = ((codepoint & 0x0f) << 12) | ((((unsigned char)*(c + 1)) & 0x3f) << 6) | (((unsigned char)*(c + 2)) & 0x3f);
                        c += 2;
                    }
                    else if ((codepoint & 0xf8) == 0xf0 && c + 3 <= &value.back() && (((unsigned char)*(c + 1)) & 0xc0) == 0x80 && (((unsigned char)*(c + 2)) & 0xc0) == 0x80 && (((unsigned char)*(c + 3)) & 0xc0) == 0x80)
                    {
                        codepoint = ((codepoint & 7) << 18) | ((((unsigned char)*(c + 1)) & 0x3f) << 12) | ((((unsigned char)*(c + 2)) & 0x3f) << 6) | (((unsigned char)*(c + 3)) & 0x3f);
                        c += 3;
                    }
                    else
                    {
                        // Invalid UTF-8 sequence.
                        Log(ComponentJSON, Trace, "Invalid UTF-8 sequence in string `%s`.", hex(value.substr(c-&value.front())).c_str());
                        return StringSharedPtr();
                    }
                }
                if (codepoint >= 0x110000 || (codepoint >= 0xd800 && codepoint <= 0xdfff))
                {
                    // Will never happen probably.
                    Log(ComponentJSON, Trace, "Invalid Unicode codepoint in string `%s`: %d.", hex(value.substr(c-&value.front())).c_str(), codepoint);
                    return StringSharedPtr();
                }
                else if (codepoint >= 0x10000 && codepoint <= 0x10ffff)
                {
                    char buf[13];
                    codepoint -= 0x10000;
                    _snprintf(buf, sizeof(buf), "\\u%04x\\u%04x", (codepoint >> 10) | 0xd800, (codepoint & 0x3ff) | 0xdc00);
                    escaped_and_encoded += buf;
                }
                else if (codepoint < 0x20 || codepoint > 0x7f)
                {
                    char buf[7];
                    _snprintf(buf, sizeof(buf), "\\u%04x", codepoint);
                    escaped_and_encoded += buf;
                }
                else
                {
                    escaped_and_encoded += (char)codepoint;
                }
        }
    }
    escaped_and_encoded += '"';
    return make_shared(escaped_and_encoded);
}

StringSharedPtr JSON::to_string() const
{
    switch (value_type)
    {
        case JSONNull:
            return make_shared(string("null"));

        case JSONBoolean:
            return make_shared(string(value.boolean ? "true" : "false"));

        case JSONNumber:
            return make_shared(*value.string_or_number);

        case JSONString:
            return to_json_string(*value.string_or_number);

        case JSONArray:
        {
            string result = "[";
            for (size_t i = 0; i < value.array->size(); ++i)
            {
                if (i > 0)
                {
                    result += ", ";
                }
                StringSharedPtr value_str = (*value.array)[i].to_string();
                if (!value_str)
                {
                    return value_str;
                }
                result += *value_str;
            }
            result += ']';
            return make_shared(result);
        }

        case JSONObject:
        {
            string result = "{";
            for (size_t i = 0; i < value.object->size(); ++i)
            {
                if (i > 0)
                {
                    result += ", ";
                }
                StringSharedPtr key_str = to_json_string((*value.object)[i].first);
                if (!key_str)
                {
                    return key_str;
                }
                StringSharedPtr value_str = (*value.object)[i].second.to_string();
                if (!value_str)
                {
                    return value_str;
                }
                result += *key_str + ": " + *value_str;
            }
            result += '}';
            return make_shared(result);
        }

        default:
            return StringSharedPtr();
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
            JSON result = JSON(StringJSONPairVector());
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
            JSON result(JSONVector());
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
        case '"':
        {
            ++it;
            string result;
            while (it <= &json_string.back())
            {
                switch (*it)
                {
                    case '"':
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
                            case '"': result += '"'; break;
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
                                size_t unicode_start_pos = it - json_string.data() - 5;
                                if (codepoint >= 0xdc00)
                                {
                                    pos = unicode_start_pos;
                                    return JSON::error();
                                }
                                if (codepoint >= 0xd800)
                                {
                                    // This codepoint requires a surrogate pair.
                                    if (it + 6 > &json_string.back() || *(it + 1) != '\\' || *(it + 2) != 'u')
                                    {
                                        pos = unicode_start_pos;
                                        return JSON::error();
                                    }
                                    it += 2;
                                    unsigned int high_surrogate = codepoint;
                                    codepoint = 0;
                                    for (size_t i = 0; i < 4; ++i)
                                    {
                                        ++it;
                                        if (it > &json_string.back())
                                        {
                                            pos = unicode_start_pos;
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
                                            pos = unicode_start_pos;
                                            return JSON::error();
                                        }
                                    }
                                    if (codepoint < 0xdc00 || codepoint > 0xdfff)
                                    {
                                        pos = unicode_start_pos;
                                        return JSON::error();
                                    }
                                    codepoint = ((high_surrogate & ~0xd800) << 10) + (codepoint & ~0xdc00) + 0x10000;
                                }
                                if (codepoint <= 0x7f)
                                {
                                    result += (char)codepoint;
                                }
                                else if (codepoint <= 0x7ff)
                                {
                                    result += (char)(0xc0 | ((codepoint >> 6) & 0x1f));
                                    result += (char)(0x80 | (codepoint & 0x3f));
                                }
                                else if (codepoint <= 0xffff)
                                {
                                    result += (char)(0xe0 | ((codepoint >> 12) & 0x0f));
                                    result += (char)(0x80 | ((codepoint >> 6) & 0x3f));
                                    result += (char)(0x80 | (codepoint & 0x3f));
                                }
                                else if (codepoint <= 0x10ffff)
                                {
                                    result += (char)(0xf0 | ((codepoint >> 18) & 0x07));
                                    result += (char)(0x80 | ((codepoint >> 12) & 0x3f));
                                    result += (char)(0x80 | ((codepoint >> 6) & 0x3f));
                                    result += (char)(0x80 | (codepoint & 0x3f));
                                }
                                else
                                {
                                    pos = unicode_start_pos;
                                    return JSON::error();
                                }
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
