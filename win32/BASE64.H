#ifndef BASE64_DEFINED
#define BASE64_DEFINED

#include <string>

#include <common.h>
#include <utils.h>


string base64_encode(const string & data);
StringWithError base64_decode(const string & data);

#endif // BASE64_DEFINED
