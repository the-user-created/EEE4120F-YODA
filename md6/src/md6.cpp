#include <cstring>
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>
#include "md6.h"

#define w md6_w
#define n md6_n
#define c md6_c
#define q md6_q
#define k md6_k
#define u md6_u
#define v md6_v
#define b md6_b

std::mutex mtx;

static const md6_word Q[15] = {
        0x7311c2812425cfa0ULL,
        0x6432286434aac8e7ULL,
        0xb60450e9ef68b7c1ULL,
        0xe8fb23908d9f06f1ULL,
        0xdd2e76cba691e5bfULL,
        0x0cd0d63b2c30bc41ULL,
        0x1f8ccf6823058f8aULL,
        0x54e5ed5b88e3775dULL,
        0x4ad12aae0a6d6031ULL,
        0x3e7f16bb88222e0dULL,
        0x8af8671d3fb50c2cULL,
        0x995ad1178bd25c31ULL,
        0xc878c1dd04c4b633ULL,
        0x3b72066c7a1552acULL,
        0x0d6f3522631effcbULL
};

int md6_detect_byte_order() {
    uint64_t x = 1;
    auto *cp = reinterpret_cast<unsigned char *>(&x);
    return (*cp == 1) ? 1 : 2;
}

void md6_reverse_little_endian(uint64_t *x, int count) {
    if (md6_detect_byte_order() == 1)
        for (int i = 0; i < count; ++i) {
            x[i] = (x[i] << 32) | (x[i] >> 32);
            x[i] = ((x[i] & 0x0000ffff0000ffffULL) << 16) | ((x[i] & 0xffff0000ffff0000ULL) >> 16);
            x[i] = ((x[i] & 0x00ff00ff00ff00ffULL) << 8) | ((x[i] & 0xff00ff00ff00ff00ULL) >> 8);
        }
}

static void append_bits(unsigned char *dest, unsigned int destlen, const unsigned char *src, const unsigned int srclen) {
    int i;
    uint16_t accum;
    unsigned int di, srcbytes, accumlen;
    if (srclen == 0) return;

    accum = 0;
    accumlen = 0;
    if (destlen % 8 != 0) {
        accumlen = destlen % 8;
        accum = dest[destlen / 8];
        accum = accum >> (8 - accumlen);
    }

    di = destlen / 8;
    srcbytes = (srclen + 7) / 8;

    unsigned int newbits;
    int numbits;
    unsigned char bits;
    for (i = 0; i < srcbytes; i++) {
        if (i != srcbytes - 1) {
            accum = (accum << 8) ^ src[i];
            accumlen += 8;
        } else {
            newbits = ((srclen % 8 == 0) ? 8 : (srclen % 8));
            accum = (accum << newbits) | (src[i] >> (8 - newbits));
            accumlen += newbits;
        }
        while (((i != srcbytes - 1) & (accumlen >= 8)) || ((i == srcbytes - 1) & (accumlen > 0))) {
            numbits = min(8, accumlen);
            bits = accum >> (accumlen - numbits);
            bits = bits << (8 - numbits);
            bits &= (0xff00 >> numbits);
            dest[di++] = bits;
            accumlen -= numbits;
        }
    }
}

int md6_full_init(md6_state *st, int d, unsigned char *key, int keylen, int L, int r) {
    if (!st || (key && (keylen < 0 || keylen > k * (w / 8))) || d < 1 || d > 512 || d > w * c / 2)
        return MD6_BADHASHLEN;

    memset(st, 0, sizeof(md6_state));
    st->d = d;

    if (key && keylen > 0) {
        memcpy(st->K, key, keylen);
        st->keylen = keylen;
        md6_reverse_little_endian(st->K, k);
    }

    if (L < 0 || L > 255 || r < 0 || r > 255)
        return MD6_BAD_L;

    st->L = L;
    st->r = r;
    st->initialized = 1;
    st->top = 1;
    if (L == 0) st->bits[1] = c * w;

    return MD6_SUCCESS;
}

int md6_init(md6_state *st, int d) {
    return md6_full_init(st, d, nullptr, 0, md6_default_L, 40 + (d / 4));
}

static int md6_compress_block(md6_word *C, md6_state *st, int ell, int z) {
    if (!st->initialized) return MD6_STATENOTINIT;
    if (ell < 0 || ell >= md6_max_stack_height - 1) return MD6_STACKUNDERFLOW;

    st->compression_calls++;
    if (ell == 1) {
        int size = (ell < (st->L + 1)) ? b : b - c;
        md6_reverse_little_endian(&(st->B[ell][0]), size);
    }

    int p = b * w - st->bits[ell];
    int err = md6_standard_compress(C, Q, st->K, ell, st->i_for_level[ell], st->r, st->L, z, p, st->keylen, st->d,
                                    st->B[ell]);
    if (err) return err;

    st->bits[ell] = 0;
    st->i_for_level[ell]++;
    memset(&(st->B[ell][0]), 0, b * sizeof(md6_word));

    return MD6_SUCCESS;
}

static int md6_process(md6_state *st, int ell, int final) {
    if (st == nullptr || st->initialized == 0) return MD6_NULLSTATE;

    if (!final && st->bits[ell] < b * w) return MD6_SUCCESS;
    if (final && ell == st->top && ((ell == (st->L + 1) && st->bits[ell] == c * w && st->i_for_level[ell] > 0) ||
                                    (ell > 1 && st->bits[ell] == c * w)))
        return MD6_SUCCESS;

    md6_word C[c];
    int z = (final && (ell == st->top)) ? 1 : 0;
    if (int err = md6_compress_block(C, st, ell, z)) return err;

    if (z == 1) {
        memcpy(st->hashval, C, md6_c * (w / 8));
        return MD6_SUCCESS;
    }

    int next_level = min(ell + 1, st->L + 1);
    if (next_level == st->L + 1 && st->i_for_level[next_level] == 0 && st->bits[next_level] == 0)
        st->bits[next_level] = c * w;

    memcpy((char *) st->B[next_level] + st->bits[next_level] / 8, C, c * (w / 8));
    st->bits[next_level] += c * w;
    if (next_level > st->top) st->top = next_level;

    return md6_process(st, next_level, final);
}

int md6_update_parallel(md6_state *st, const unsigned char *data, uint64_t databitlen) {
    if (st == nullptr) return MD6_NULLSTATE;
    if (st->initialized == 0) return MD6_STATENOTINIT;
    if (data == nullptr) return MD6_NULLDATA;

    unsigned int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    std::vector<md6_state> states(num_threads);

    unsigned int chunk_size = databitlen / num_threads;

    // Initialize individual states
    for (unsigned int i = 0; i < num_threads; ++i) {
        md6_init(&states[i], st->d);
        states[i].L = st->L;
        states[i].r = st->r;
        std::memcpy(states[i].K, st->K, sizeof(st->K));
        states[i].keylen = st->keylen;
    }

    // Define a lambda function to process a chunk of data
    auto process_chunk = [&](unsigned int thread_id, unsigned int start_bit, unsigned int end_bit) {
        unsigned int portion_size;
        unsigned char *dest;
        const unsigned char *src;
        int err;

        for (unsigned int j = start_bit; j < end_bit;) {
            portion_size = min(end_bit - j, static_cast<unsigned int>(b * w - (states[thread_id].bits[1])));
            dest = (unsigned char *)states[thread_id].B[1] + states[thread_id].bits[1] / 8;
            src = &(data[j / 8]);

            if ((portion_size % 8 == 0) && (states[thread_id].bits[1] % 8 == 0) && (j % 8 == 0)) {
                std::memcpy(dest, src, portion_size / 8);
            } else {
                append_bits(dest, states[thread_id].bits[1], src, portion_size);
            }

            j += portion_size;
            states[thread_id].bits[1] += portion_size;
            states[thread_id].bits_processed += portion_size;

            if (states[thread_id].bits[1] == b * w && j < end_bit) {
                err = md6_process(&states[thread_id], 1, 0);
                if (err) {
                    std::lock_guard<std::mutex> lock(mtx);
                    return;
                }
            }
        }
    };

    // Create threads to process each chunk
    for (unsigned int i = 0; i < num_threads; ++i) {
        unsigned int start_bit = i * chunk_size;
        unsigned int end_bit = (i == num_threads - 1) ? databitlen : start_bit + chunk_size;
        threads.emplace_back(process_chunk, i, start_bit, end_bit);
    }

    // Wait for all threads to complete
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Combine results from all threads
    for (unsigned int i = 0; i < num_threads; ++i) {
        if (states[i].bits[1] > 0) {
            unsigned int portion_size = states[i].bits[1];
            unsigned char *dest = (unsigned char *)st->B[1] + st->bits[1] / 8;
            unsigned char *src = (unsigned char *)states[i].B[1];

            std::memcpy(dest, src, portion_size / 8);
            st->bits[1] += portion_size;
            st->bits_processed += portion_size;

            if (st->bits[1] == b * w) {
                int err = md6_process(st, 1, 0);
                if (err) return err;
            }
        }
    }

    return MD6_SUCCESS;
}

int md6_update(md6_state *st, const unsigned char *data, uint64_t databitlen) {
    if (st == nullptr) return MD6_NULLSTATE;
    if (st->initialized == 0) return MD6_STATENOTINIT;
    if (data == nullptr) return MD6_NULLDATA;

    unsigned int portion_size;
    unsigned char *dest;
    const unsigned char *src;
    int err;
    for (unsigned int j = 0; j < databitlen;) {
        portion_size = min(databitlen - j, (unsigned int) (b * w - (st->bits[1])));
        dest = (unsigned char *) st->B[1] + st->bits[1] / 8;
        src = &(data[j / 8]);

        if ((portion_size % 8 == 0) && (st->bits[1] % 8 == 0) && (j % 8 == 0)) {
            memcpy(dest, src, portion_size / 8);
        } else {
            append_bits(dest, st->bits[1], src, portion_size);
        }

        j += portion_size;
        st->bits[1] += portion_size;
        st->bits_processed += portion_size;

        if (st->bits[1] == b * w && j < databitlen) {
            err = md6_process(st, 1, 0);
            if (err) return err;
        }
    }

    return MD6_SUCCESS;
}

static int md6_compute_hex_hashval(md6_state *st) {
    const unsigned char hexDigits[] = "0123456789abcdef";
    int hashLength = (st->d + 7) / 8;

    for (int i = 0; i < hashLength; i++) {
        st->hexhashval[2 * i] = hexDigits[(st->hashval[i] >> 4) & 0xf];
        st->hexhashval[2 * i + 1] = hexDigits[st->hashval[i] & 0xf];
    }

    st->hexhashval[(st->d + 3) / 4] = 0;
    return MD6_SUCCESS;
}

static void trim_hashval(md6_state *st) {
    int full_or_partial_bytes = (st->d + 7) / 8;
    int bits = st->d % 8;

    // Move the relevant bytes to the beginning of the array
    for (int i = 0; i < full_or_partial_bytes; i++)
        st->hashval[i] = st->hashval[c * (w / 8) - full_or_partial_bytes + i];

    // Zero out the rest of the array
    memset(&st->hashval[full_or_partial_bytes], 0, (c * (w / 8) - full_or_partial_bytes) * sizeof(st->hashval[0]));

    // If there are extra bits, shift and combine bytes
    if (bits > 0) {
        for (int i = 0; i < full_or_partial_bytes; i++) {
            st->hashval[i] = (st->hashval[i] << (8 - bits));
            if ((i + 1) < c * (w / 8))
                st->hashval[i] |= (st->hashval[i + 1] >> bits);
        }
    }
}

int find_first_non_zero_bit(md6_state *st) {
    for (int ell = 1; ell <= st->top; ell++)
        if (st->bits[ell] > 0) return ell;
    return 1;
}

int md6_final(md6_state *st, unsigned char *hashval) {
    if (st == nullptr) return MD6_NULLSTATE;
    if (!st->initialized) return MD6_STATENOTINIT;
    if (st->finalized) return MD6_SUCCESS;

    int ell = (st->top == 1) ? 1 : find_first_non_zero_bit(st);
    int err = md6_process(st, ell, 1);
    if (err) return err;

    md6_reverse_little_endian((md6_word *) st->hashval, c);
    trim_hashval(st);

    if (hashval != nullptr) memcpy(hashval, st->hashval, (st->d + 7) / 8);
    md6_compute_hex_hashval(st);

    st->finalized = 1;
    return MD6_SUCCESS;
}
