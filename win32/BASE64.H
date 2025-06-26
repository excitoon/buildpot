#ifndef BASE64_DEFINED
#define BASE64_DEFINED

#include <string>
#include <utility>

#include <common.h>


string base64_encode(const string & data);
StringBoolPair base64_decode(const string & data);

#endif // BASE64_DEFINED
