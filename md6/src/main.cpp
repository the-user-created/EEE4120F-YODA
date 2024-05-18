#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <chrono>
#include "md6.h"

std::string md6Hash(const char *inputS, int hashBitLen, bool is_parallel) {
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

    if (is_parallel) {
        md6_update_parallel(ctx, input, strlen(inputS) * 8);
    } else {
        md6_update(ctx, input, strlen(inputS) * 8);
    }

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
    double total_time = 0;
    int executions = 1000;
    std::vector<double> times(executions, 0); // Vector to store all execution times

    bool is_parallel = false;

    std::cout << "Running " << executions << " executions of MD6-128 hashing with " <<
              (is_parallel ? "parallel" : "sequential") << " implementation\n";

    for (int i = 0; i < executions; ++i) {
        auto start = std::chrono::high_resolution_clock::now();

        std::string hash128 = md6Hash("test", 128, is_parallel);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;

        total_time += diff.count();
        times[i] = diff.count(); // Store execution time in vector
    }

    double average_time = total_time / executions;

    // Calculate min, max and stddev
    double min_time = *std::min_element(times.begin(), times.end());
    double max_time = *std::max_element(times.begin(), times.end());
    double sum_diff_sq = std::accumulate(times.begin(), times.end(), 0.0,
                                         [average_time](double a, double b) { return a + pow(b - average_time, 2); });
    double stddev_time = sqrt(sum_diff_sq / executions);

    std::cout << "Average time: " << average_time * 1000 << " ms\n";
    std::cout << "Min time: " << min_time * 1000 << " ms\n";
    std::cout << "Max time: " << max_time * 1000 << " ms\n";
    std::cout << "Standard deviation: " << stddev_time * 1000 << " ms\n";

    return 0;
}
