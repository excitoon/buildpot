#include <sha2.h>

#include <string>


const unsigned long sha2_k[64] =
{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
    0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
    0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
    0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
    0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
    0xc67178f2,
};

unsigned long rotr(unsigned long x, unsigned long n)
{
    return (x >> n) | (x << (32 - n));
}

string sha256(const string & data)
{
    string msg(data);
    unsigned long bitlen = msg.size() * 8;
    msg.push_back((char)0x80);
    while ((msg.size() % 64) != 56)
    {
        msg.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i)
    {
        msg.push_back((char)((bitlen >> (i * 8)) & 0xff));
    }

    unsigned long h[8] =
    {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c,
        0x1f83d9ab, 0x5be0cd19,
    };

    for (size_t chunk = 0; chunk < msg.size(); chunk += 64)
    {
        unsigned long w[64];
        for (int i = 0; i < 16; ++i)
        {
            w[i] = ((unsigned char)(msg[chunk + 4*i]) << 24) |
                   ((unsigned char)(msg[chunk + 4*i + 1]) << 16) |
                   ((unsigned char)(msg[chunk + 4*i + 2]) << 8) |
                   ((unsigned char)(msg[chunk + 4*i + 3]));
        }
        for (i = 16; i < 64; ++i)
        {
            unsigned long s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
            unsigned long s1 = rotr(w[i-2], 17) ^ rotr(w[i-2], 19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }

        unsigned long a = h[0], b = h[1], c = h[2], d = h[3];
        unsigned long e = h[4], f = h[5], g = h[6], h_ = h[7];

        for (i = 0; i < 64; ++i)
        {
            unsigned long S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            unsigned long ch = (e & f) ^ ((~e) & g);
            unsigned long temp1 = h_ + S1 + ch + sha2_k[i] + w[i];
            unsigned long S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            unsigned long maj = (a & b) ^ (a & c) ^ (b & c);
            unsigned long temp2 = S0 + maj;

            h_ = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += h_;
    }

    string result;
    for (i = 0; i < 8; ++i)
    {
        for (int j = 3; j >= 0; --j)
        {
            result.push_back((char)((h[i] >> (8 * j)) & 0xff));
        }
    }
    return result;
}
