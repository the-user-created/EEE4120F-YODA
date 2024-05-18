#include <cstdlib>
#include <cstring>
#include "md6.h"

// Define constants
#define w md6_w
#define n md6_n
#define c md6_c
#define b md6_b
#define v md6_v
#define u md6_u
#define k md6_k
#define q md6_q

int RL[16][2] = {
        {10, 11}, {5, 24}, {13, 9}, {10, 16}, {11, 15}, {12, 9},
        {2, 27}, {7, 15}, {14, 6}, {15, 2}, {7, 29}, {13, 8},
        {11, 15}, {7, 5}, {6, 31}, {12, 9}
};

// Main compression loop
static void md6_main_compression_loop(md6_word *A, int r) {
    md6_word x, S = 0x0123456789abcdefULL;
    int i = n;

    for (int j = 0; j < r * c; j += c) {
        for (int step = 0; step < 16; step++) {
            x = S;
            x ^= A[i + step - 89];
            x ^= A[i + step - 17];
            x ^= (A[i + step - 18] & A[i + step - 21]);
            x ^= (A[i + step - 31] & A[i + step - 67]);
            x ^= (x >> RL[step][0]);
            A[i + step] = x ^ (x << RL[step][1]);
        }
        S = (S << 1) ^ (S >> (w - 1)) ^ (S & 0x7311c2812425cfa0ULL);
        i += 16;
    }
}

// Compression function
static int md6_compress(md6_word *C, md6_word *N, int r, md6_word *A) {
    if (!N || !C || r < 0 || r > md6_max_r) return MD6_BAD_r;

    md6_word *A_as_given = A;
    if (!A) {
        A = (md6_word *) calloc(r * c + n, sizeof(md6_word));
        if (!A) return MD6_OUT_OF_MEMORY;
    }

    memcpy(A, N, n * sizeof(md6_word));
    md6_main_compression_loop(A, r);
    memcpy(C, A + (r - 1) * c + n, c * sizeof(md6_word));

    if (!A_as_given) {
        memset(A, 0, (r * c + n) * sizeof(md6_word));
        free(A);
    }

    return MD6_SUCCESS;
}

// Create control word
static md6_control_word md6_make_control_word(int r, int L, int z, int p, int keylen, int d) {
    return (((md6_control_word) 0 << 60) |
            ((md6_control_word) r << 48) |
            ((md6_control_word) L << 40) |
            ((md6_control_word) z << 36) |
            ((md6_control_word) p << 20) |
            ((md6_control_word) keylen << 12) |
            (md6_control_word) d);
}

// Standard compress function
int md6_standard_compress(md6_word *C, const md6_word *Q, const md6_word *K, int ell, int i, int r, int L, int z,
                                 int p, int keylen, int d, md6_word *B) {
    if (!C || !B || !K || !Q) return MD6_NULL_C;
    if (r < 0 || r > md6_max_r || L < 0 || L > 255 || ell < 0 || ell > 255
        || p < 0 || p > b * w || d <= 0 || d > c * w / 2)
        return MD6_BAD_r;

    md6_word N[md6_n];
    md6_word A[5000];

    // Pack
    int ni = 0;

    for (int j = 0; j < q; j++) N[ni++] = Q[j];
    for (int j = 0; j < k; j++) N[ni++] = K[j];

    md6_nodeID U = ((md6_nodeID) ell << 56) | i;
    memcpy((unsigned char *) &N[ni], &U, min(u * (w / 8), sizeof(md6_nodeID)));
    ni += u;

    md6_control_word V = md6_make_control_word(r, L, z, p, keylen, d);
    memcpy((unsigned char *) &N[ni], &V, min(v * (w / 8), sizeof(md6_control_word)));
    ni += v;

    memcpy(N + ni, B, b * sizeof(md6_word));

    return md6_compress(C, N, r, A);
}
