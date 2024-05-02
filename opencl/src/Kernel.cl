#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

__constant uint s[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, // Round 1
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,     // Round 2
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,  // Round 3
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21   // Round 4
};

__constant uint K[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcef3f8c, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};


__kernel void md5_hash(__global const uchar *input, __global uchar *output, const ulong length) {
    // Define constants and MD5 auxiliary functions here

    // Each work-item processes a 512-bit block
    int idx = get_global_id(0) * 64;  // Corrected indexing to account for 512-bit blocks

    // Ensure we do not go out of bounds
    if (idx < length) {
        // Initialize the MD5 state once per kernel invocation
        uint a0 = 0x67452301;
        uint b0 = 0xefcdab89;
        uint c0 = 0x98badcfe;
        uint d0 = 0x10325476;

        // Main MD5 transformation loop
        for(int i = 0; i < 64; i++) {
            uint f, g;
            if (i < 16) {
                f = (b0 & c0) | ((~b0) & d0);
                g = i;
            } else if (i < 32) {
                f = (d0 & b0) | ((~d0) & c0);
                g = (5 * i + 1) % 16;
            } else if (i < 48) {
                f = b0 ^ c0 ^ d0;
                g = (3 * i + 5) % 16;
            } else {
                f = c0 ^ (b0 | (~d0));
                g = (7 * i) % 16;
            }

            f = f + a0 + K[i] + input[idx + g];
            a0 = d0;
            d0 = c0;
            c0 = b0;
            b0 = b0 + LEFTROTATE(f, s[i]);
        }

        // Output the result as bytes
        output[idx / 4 + 0] = (uchar)(a0 & 0xFF);
        output[idx / 4 + 1] = (uchar)((a0 >> 8) & 0xFF);
        output[idx / 4 + 2] = (uchar)((a0 >> 16) & 0xFF);
        output[idx / 4 + 3] = (uchar)((a0 >> 24) & 0xFF);

        output[idx / 4 + 4] = (uchar)(b0 & 0xFF);
        output[idx / 4 + 5] = (uchar)((b0 >> 8) & 0xFF);
        output[idx / 4 + 6] = (uchar)((b0 >> 16) & 0xFF);
        output[idx / 4 + 7] = (uchar)((b0 >> 24) & 0xFF);

        output[idx / 4 + 8] = (uchar)(c0 & 0xFF);
        output[idx / 4 + 9] = (uchar)((c0 >> 8) & 0xFF);
        output[idx / 4 + 10] = (uchar)((c0 >> 16) & 0xFF);
        output[idx / 4 + 11] = (uchar)((c0 >> 24) & 0xFF);

        output[idx / 4 + 12] = (uchar)(d0 & 0xFF);
        output[idx / 4 + 13] = (uchar)((d0 >> 8) & 0xFF);
        output[idx / 4 + 14] = (uchar)((d0 >> 16) & 0xFF);
        output[idx / 4 + 15] = (uchar)((d0 >> 24) & 0xFF);
    }
}
