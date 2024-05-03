// Define helper functions (similar to C++ version, adapted for OpenCL)
static uint F(uint x, uint y, uint z) {
    return (x & y) | (~x & z);
}

static uint G(uint x, uint y, uint z) {
    return (x & z) | (y & ~z);
}

static uint H(uint x, uint y, uint z) {
    return x ^ y ^ z;
}

static uint I(uint x, uint y, uint z) {
    return y ^ (x | ~z);
}

__kernel void md5_hash(__global unsigned char* input, __global unsigned char* output, uint inputSize) {
    // CHECKS:
    // input is passed correctly
    // inputSize is correct
    // S, K, A, B, C, D are correct

    // Constants for MD5 transform
    const uint S[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                        5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

    const uint K[64] = {0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
                        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
                        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
                        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                        0xd62f105d, 0x2441453, 0xd8a1e681, 0xe7d3fbc8,
                        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
                        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
                        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
                        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x4881d05,
                        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
                        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
                        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

    // Initialize variables
    uint A = 0x67452301;
    uint B = 0xefcdab89;
    uint C = 0x98badcfe;
    uint D = 0x10325476;

    // Step 5: Process Message in 16-Word Blocks
    for (uint i = 0; i < inputSize; i += 64) {
        uint M[16];
        for (int j = 0; j < 16; ++j) {
            M[j] = (input[i + j*4 + 3] << 24) | (input[i + j*4 + 2] << 16) | (input[i + j*4 + 1] << 8) | input[i + j*4];
        }

        uint AA = A;
        uint BB = B;
        uint CC = C;
        uint DD = D;

        // Main loop
        for (int j = 0; j < 64; ++j) {
            uint tempF, g;

            if (j <= 15) {
                /* Round 1. */
                /* Let [abcd k s i] denote the operation a = b + ((a + F(b,c,d) + X[k] + T[i]) <<< s). */
                tempF = F(BB, CC, DD);
                g = j;
            } else if (j <= 31) {
                /* Round 2. a = b + ((a + G(b,c,d) + X[k] + T[i]) <<< s).*/
                tempF = G(BB, CC, DD);
                g = (5*j + 1) % 16;
            } else if (j <= 47) {
                /* Round 3. a = b + ((a + H(b,c,d) + X[k] + T[i]) <<< s).*/
                tempF = H(BB, CC, DD);
                g = (3*j + 5) % 16;
            } else {
                /* Round 4. a = b + ((a + I(b,c,d) + X[k] + T[i]) <<< s).*/
                tempF = I(BB, CC, DD);
                g = (7*j) % 16;
            }

            tempF = tempF + AA + K[j] + M[g]; // Note: Addition may overflow, which is fine
            AA = DD;
            DD = CC;
            CC = BB;
            BB += (tempF << S[j]) | (tempF >> (32 - S[j])); // Note: The rotation is separate from the addition to prevent recomputation
        }

        // Add this chunk's hash to result so far
        A += AA;
        B += BB;
        C += CC;
        D += DD;
    }

    // Output the final hash
    for (int i = 0; i < 4; ++i) {
        output[i]     = (uchar)(A >> (i * 8));
        output[i + 4] = (uchar)(B >> (i * 8));
        output[i + 8] = (uchar)(C >> (i * 8));
        output[i + 12] = (uchar)(D >> (i * 8));
    }
}
