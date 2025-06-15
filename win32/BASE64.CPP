#include <base64.h>


string base64_encode(const string & data)
{
    static const char * base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    string encoded;
    size_t len = data.size();
    size_t padding = (3 - (len % 3)) % 3;
    size_t encoded_length = 4 * ((len + 2) / 3);
    encoded.reserve(encoded_length);

    for (size_t i = 0; i < len; i += 3)
    {
        unsigned long triplet = (data[i] << 16) | (i + 1 < len ? data[i + 1] << 8 : 0) | (i + 2 < len ? data[i + 2] : 0);
        encoded.push_back(base64_chars[(triplet >> 18) & 0x3F]);
        encoded.push_back(base64_chars[(triplet >> 12) & 0x3F]);
        encoded.push_back(i + 1 < len ? base64_chars[(triplet >> 6) & 0x3F] : '=');
        encoded.push_back(i + 2 < len ? base64_chars[triplet & 0x3F] : '=');
    }

    return encoded;
}

string base64_decode(const string & data)
{
    // FIXME exceptions
    static const unsigned char base64_decode_table[256] = {
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, //   0-15
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, //  16-31
        64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63, //  32-47  '+'=43, '/'=47
        52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64, //  48-63  '0'-'9'=48-57
        64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, //  64-79  'A'-'O'=65-79
        15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64, //  80-95  'P'-'Z'=80-90
        64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40, //  96-111 'a'-'o'=97-111
        41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64, // 112-127 'p'-'z'=112-122
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, // 128-143
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, // 144-159
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, // 160-175
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, // 176-191
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, // 192-207
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, // 208-223
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, // 224-239
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64  // 240-255
    };
    string decoded;
    size_t len = data.size();
    if (len % 4 != 0) return decoded;

    decoded.reserve((len / 4) * 3);
    for (size_t i = 0; i < len; i += 4)
    {
        unsigned long sextet_a = data[i] == '=' ? 0 : base64_decode_table[(unsigned char)data[i]];
        unsigned long sextet_b = data[i + 1] == '=' ? 0 : base64_decode_table[(unsigned char)data[i + 1]];
        unsigned long sextet_c = data[i + 2] == '=' ? 0 : base64_decode_table[(unsigned char)data[i + 2]];
        unsigned long sextet_d = data[i + 3] == '=' ? 0 : base64_decode_table[(unsigned char)data[i + 3]];

        unsigned long triplet = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;
        decoded.push_back((char)((triplet >> 16) & 0xFF));
        if (data[i + 2] != '=') decoded.push_back((char)((triplet >> 8) & 0xFF));
        if (data[i + 3] != '=') decoded.push_back((char)(triplet & 0xFF));
    }

    return decoded;
}
