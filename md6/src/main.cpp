#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <fstream>
#include <sstream>
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

void runTests(bool is_parallel) {
    std::cout << "Running tests for " << (is_parallel ? "parallel" : "sequential") << " implementation\n";

    int executions = 100;
    std::vector<double> times(executions, 0); // Vector to store all execution times

    // Loop over different input sizes
    for (unsigned long long inputSize = 0; inputSize <= pow(2, 23); inputSize += 4 * ceil(pow(2, 23) / 400)) {
        // Generate input string of the required size
        std::string inputS(inputSize, 'a'); // Fill the string with 'a'

        std::cout << "Running " << executions << " executions of MD6-128 hashing with " <<
                  (is_parallel ? "parallel" : "sequential") << " implementation on input size " << inputSize << "\n";

        for (int i = 0; i < executions; ++i) {
            auto start = std::chrono::high_resolution_clock::now();

            std::string hash128 = md6Hash(inputS.c_str(), 128, is_parallel);

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> diff = end - start;

            times[i] = diff.count(); // Store execution time in vector
        }

        // Determine the output file name based on is_parallel
        std::string outputFileName = is_parallel ? "times_par.csv" : "times_seq.csv";

        // Write the execution times to a CSV file
        std::ofstream outputFile(outputFileName, std::ios_base::app); // Append to the file
        if (inputSize == 0) {
            outputFile << "Run Number,Message Size,Execution Time\n"; // Write the headers
        }
        for (int i = 0; i < executions; ++i) {
            outputFile << i+1 << "," << inputSize << "," << times[i] << "\n";
        }
        outputFile.close();

        // Clear the vector
        times.clear();
    }

    std::cout << "Finished running tests for " << (is_parallel ? "parallel" : "sequential") << " implementation\n";
}

void singleTestSequential() {
    std::cout << "Running single test for sequential implementation" << std::endl;

    // 128-bit hash
    auto start = std::chrono::high_resolution_clock::now();

    std::string hash128 = md6Hash("test", 128, false);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::string knownHash128 = "a133b0efa199156be653427c6ab85d3d";

    if (hash128 == knownHash128) {
        std::cout << "MD6-128 hash matches known hash." << std::endl;
        std::cout << "Execution time: " << diff.count() << "s" << std::endl;
    } else {
        std::cout << "MD6-128 hash does not match known hash." << std::endl;
    }

    // 256-bit hash
    start = std::chrono::high_resolution_clock::now();

    std::string hash256 = md6Hash("test", 256, false);

    end = std::chrono::high_resolution_clock::now();
    diff = end - start;

    std::string knownHash256 = "93c8a7d0ff132f325138a82b2baa98c12a7c9ac982feb6c5b310a1ca713615bd";

    if (hash256 == knownHash256) {
        std::cout << "MD6-256 hash matches known hash." << std::endl;
        std::cout << "Execution time: " << diff.count() << "s" << std::endl;
    } else {
        std::cout << "MD6-256 hash does not match known hash." << std::endl;
    }

    // 512-bit hash
    start = std::chrono::high_resolution_clock::now();

    std::string hash512 = md6Hash("test", 512, false);
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;

    std::string knownHash512 = "d96ce883f4632f826b3bb553fe5cbff8fb00b32b3534b39aa0c0899d1199a8cf28d77e49f2465517dfb12c0f3268b90f8a13d94e6730a74ed2e8312242a9e937";

    if (hash512 == knownHash512) {
        std::cout << "MD6-512 hash matches known hash." << std::endl;
        std::cout << "Execution time: " << diff.count() << "s" << std::endl;
    } else {
        std::cout << "MD6-512 hash does not match known hash." << std::endl;
    }

    std::cout << "" << std::endl;
}


void singleTestParallel() {
    std::cout << "Running single test for parallel implementation" << std::endl;

    // 128-bit hash
    auto start = std::chrono::high_resolution_clock::now();

    std::string hash128 = md6Hash("test", 128, true);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::string knownHash128 = "a133b0efa199156be653427c6ab85d3d";

    if (hash128 == knownHash128) {
        std::cout << "MD6-128 hash matches known hash." << std::endl;
        std::cout << "Execution time: " << diff.count() << "s" << std::endl;
    } else {
        std::cout << "MD6-128 hash does not match known hash." << std::endl;
    }

    // 256-bit hash
    start = std::chrono::high_resolution_clock::now();

    std::string hash256 = md6Hash("test", 256, true);

    end = std::chrono::high_resolution_clock::now();
    diff = end - start;

    std::string knownHash256 = "93c8a7d0ff132f325138a82b2baa98c12a7c9ac982feb6c5b310a1ca713615bd";

    if (hash256 == knownHash256) {
        std::cout << "MD6-256 hash matches known hash." << std::endl;
        std::cout << "Execution time: " << diff.count() << "s" << std::endl;
    } else {
        std::cout << "MD6-256 hash does not match known hash." << std::endl;
    }

    // 512-bit hash
    start = std::chrono::high_resolution_clock::now();

    std::string hash512 = md6Hash("test", 512, true);
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;

    std::string knownHash512 = "d96ce883f4632f826b3bb553fe5cbff8fb00b32b3534b39aa0c0899d1199a8cf28d77e49f2465517dfb12c0f3268b90f8a13d94e6730a74ed2e8312242a9e937";

    if (hash512 == knownHash512) {
        std::cout << "MD6-512 hash matches known hash." << std::endl;
        std::cout << "Execution time: " << diff.count() << "s" << std::endl;
    } else {
        std::cout << "MD6-512 hash does not match known hash." << std::endl;
    }

    std::cout << "" << std::endl;
}


int main() {
    // Run the tests for both parallel and sequential implementations
    // runTests(true);
    // runTests(false);

    // Run sequential verification tests
    singleTestSequential();

    // Run parallel verification tests
    singleTestParallel();

    return 0;
}
