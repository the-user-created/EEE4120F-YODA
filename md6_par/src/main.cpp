#include <iostream>
#include <cstring>
#include <cstdlib>
#include "md6.h"

std::string md6Hash(const char *inputS, int hashBitLen) {
    // Convert nibble to hex
    auto nibble2hex = [](unsigned char nibble) {
        if (nibble < 10)
            return '0' + nibble;
        else
            return 'a' + nibble - 10;
    };

    // Convert digest to hex
    auto hexDigest = [&nibble2hex](const unsigned char *digest, char *hexdigest, int byteLen) {
        for (unsigned int i = 0, h = 0; i < byteLen; i++) {
            int hi = (digest[i] & 0xF0) >> 4;
            int lo = digest[i] & 0x0F;
            hexdigest[h++] = nibble2hex(hi);
            hexdigest[h++] = nibble2hex(lo);
        }
    };

    int hashByteLen = hashBitLen / 8;
    const unsigned char *input = (unsigned char *) inputS;

    auto *ctx = (md6_state *) malloc(sizeof(md6_state));
    md6_init(ctx, hashBitLen);

    md6_update(ctx, input, strlen(inputS) * 8);

    auto *output = (unsigned char *) malloc(hashByteLen);
    md6_final(ctx, output);
    char *outputh = (char *) malloc(hashByteLen * 2 + 1);
    outputh[hashByteLen * 2] = 0;
    hexDigest(output, outputh, hashByteLen);
    std::string hash(outputh);

    free(output);
    free(outputh);
    free(ctx);

    return hash;
}

int main() {
    std::string hash128 = md6Hash("test", 128);
    std::string hash256 = md6Hash("test", 256);
    std::string hash512 = md6Hash("test", 512);

    std::string knownHash128 = "a133b0efa199156be653427c6ab85d3d";
    std::string knownHash256 = "93c8a7d0ff132f325138a82b2baa98c12a7c9ac982feb6c5b310a1ca713615bd";
    std::string knownHash512 = "d96ce883f4632f826b3bb553fe5cbff8fb00b32b3534b39aa0c0899d1199a8cf28d77e49f2465517dfb12c0f3268b90f8a13d94e6730a74ed2e8312242a9e937";

    if (hash128 == knownHash128) {
        std::cout << "MD6-128 hash matches known hash." << std::endl;
    } else {
        std::cout << "MD6-128 hash does not match known hash." << std::endl;
        std::cout << "Expected: " << knownHash128 << std::endl;
        std::cout << "Actual: " << hash128 << std::endl;
    }

    if (hash256 == knownHash256) {
        std::cout << "MD6-256 hash matches known hash." << std::endl;
    } else {
        std::cout << "MD6-256 hash does not match known hash." << std::endl;
        std::cout << "Expected: " << knownHash256 << std::endl;
        std::cout << "Actual: " << hash256 << std::endl;
    }

    if (hash512 == knownHash512) {
        std::cout << "MD6-512 hash matches known hash." << std::endl;
    } else {
        std::cout << "MD6-512 hash does not match known hash." << std::endl;
        std::cout << "Expected: " << knownHash512 << std::endl;
        std::cout << "Actual: " << hash512 << std::endl;
    }

    return 0;
}
