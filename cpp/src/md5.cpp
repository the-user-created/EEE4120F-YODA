/*
 * Author: Caide Marc Spriestersbach
 * Student Number: SPRCAI002
 * Date: 2 May 2024
 * Project Title: C++ MD5 Hashing Library
 * Description: Various implementations of the MD5 hashing algorithm. This implementation is based on the MD5 algorithm described in RFC 1321. This also solely focuses on the C++ implementation of the MD5 algorithm.
 */

#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>

// Define a typedef for a function pointer that takes three uint32_t and returns a uint32_t
typedef uint32_t (*FuncPtr)(uint32_t, uint32_t, uint32_t);

// The rotation amounts for each round
static const constexpr uint32_t S[64] = {
        7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

// The constants for each round
static const constexpr uint32_t K[64] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
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
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

// The g values for each round
static const constexpr uint32_t g_values[64] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, // for j <= 15
        1, 6, 11, 0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, // for j <= 31
        5, 8, 11, 14, 1, 4, 7, 10, 13, 0, 3, 6, 9, 12, 15, 2, // for j <= 47
        0, 7, 14, 5, 12, 3, 10, 1, 8, 15, 6, 13, 4, 11, 2, 9  // for j <= 63
};

uint32_t a0 = 0x67452301; // Initial value of 'a' where 'a' is a 32-bit word.
uint32_t b0 = 0xefcdab89; // Initial value of 'b' where 'b' is a 32-bit word.
uint32_t c0 = 0x98badcfe; // Initial value of 'c' where 'c' is a 32-bit word.
uint32_t d0 = 0x10325476; // Initial value of 'd' where 'd' is a 32-bit word.

// In each bit position F acts as a conditional: if X then Y else Z. The expression for F is: F(X,Y,Z) = XY v not(X) Z
inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) | (~x & z);
}

// The expression for G is: G(X,Y,Z) = XZ v Y not(Z)
inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) {
    return (x & z) | (y & ~z);
}

// The expression for H is: H(X,Y,Z) = X xor Y xor Z
inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) {
    return x ^ y ^ z;
}

// The expression for I is: I(X,Y,Z) = Y xor (X v not(Z))
inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) {
    return y ^ (x | ~z);
}

// Define the lookup table
FuncPtr funcTable[4] = {F, G, H, I};

// Calculate the MD5 hash of the input message
std::array<uint8_t, 16> calculate(const std::string& inputStr) {
    std::vector<uint8_t> input(inputStr.begin(), inputStr.end());
    uint64_t bitLen = input.size() * 8; // original length in bits

    // Step 1: Append a single '1' bit
    input.push_back(0x80); // in bits: 10000000

    // Step 2: Append '0' bits until length is 448 modulo 512
    while (input.size() % 64 != 56) { // 448 = 512 - 64
        input.push_back(0); // in bits: 00000000
    }

    // Step 3: Append 64-bit representation of original length
    for (int i = 0; i < 8; ++i) { // 64 bits = 8 bytes
        input.push_back(bitLen >> (i * 8)); // append 8 bits at a time from the 64-bit length
    }

    // Step 4: Initialize MD Buffer
    // Here each of A, B, C, D is a 32-bit register. These registers will contain the final hash.
    uint32_t A = a0;
    uint32_t B = b0;
    uint32_t C = c0;
    uint32_t D = d0;

    // Step 5: Process Message in 16-Word Blocks
    for (size_t i = 0; i < input.size(); i += 64) {
        // Break chunk into sixteen 32-bit words M[j], 0 ≤ j ≤ 15
        uint32_t M[16];
        for (int j = 0; j < 16; ++j) {
            M[j] = (input[i + j*4 + 3] << 24) | (input[i + j*4 + 2] << 16) | (input[i + j*4 + 1] << 8) | input[i + j*4];
        }

        // Initialize hash value for this chunk
        // Note: The following are copies of A, B, C, D which initially are set to a0, b0, c0, and d0 respectively for the first chunk.
        uint32_t AA = A;
        uint32_t BB = B;
        uint32_t CC = C;
        uint32_t DD = D;

        // Main loop
        for (int j = 0; j < 64; j += 4) {
            uint32_t tempF[4], g[4], tempShift[4];

            // Loop unrolling to reduce overhead of loop control and increase instruction-level parallelism
            for (int k = 0; k < 4; ++k) {
                int index = (j + k) >> 4;

                // Call the appropriate function using the lookup table
                tempF[k] = funcTable[index](BB, CC, DD);

                g[k] = g_values[j + k];

                tempF[k] = tempF[k] + AA + K[j + k] + M[g[k]]; // Note: Addition may overflow, which is fine
                tempShift[k] = (tempF[k] << S[j + k]) | (tempF[k] >> (32 - S[j + k])); // Store the result of the bitwise operation in a temporary variable
                AA = DD;
                DD = CC;
                CC = BB;
                BB += tempShift[k]; // Use the stored result
            }
        }

        // Add this chunk's hash to result so far
        A += AA;
        B += BB;
        C += CC;
        D += DD;
    }

    // Step 6: Output
    // The final result is the MD5 hash of the input message
    std::array<uint8_t, 16> result;
    for (int i = 0; i < 4; ++i) {
        result[i]     = (uint8_t)(A >> (i * 8));
        result[i + 4] = (uint8_t)(B >> (i * 8));
        result[i + 8] = (uint8_t)(C >> (i * 8));
        result[i + 12] = (uint8_t)(D >> (i * 8));
    }

    return result;
}

void runTests() {
    int executions = 100;
    std::vector<double> times(executions, 0); // Vector to store all execution times

    // Loop over different input sizes
    for (unsigned long long inputSize = 0; inputSize <= pow(2, 23); inputSize += 4 * ceil(pow(2, 23) / 400)) {
        // Generate input string of the required size
        std::string inputS(inputSize, 'a'); // Fill the string with 'a'

        std::cout << "Running " << executions << " executions of MD5 hashing on input size " << inputSize << "\n";

        for (int i = 0; i < executions; ++i) {
            auto start = std::chrono::high_resolution_clock::now();

            std::array<uint8_t, 16> hash = calculate(inputS);

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> diff = end - start;

            times[i] = diff.count(); // Store execution time in vector
        }

        // Write the execution times to a CSV file
        std::ofstream outputFile("execution_times.csv", std::ios_base::app); // Append to the file
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
}

void singleTest() {
    std::string input = "The quick brown fox jumps over the lazy dog";

    auto start = std::chrono::high_resolution_clock::now();

    std::array<uint8_t, 16> hash = calculate(input);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "MD5 hash of '" << input << "': ";
    for (uint8_t byte : hash) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    std::cout << "\n";
    // Expected hash: 9e107d9d372bb6826bd81d3542a419d6

    std::cout << "Execution time: " << diff.count() << " s\n";
}


int main() {
    // Run the tests
    // runTests();

    // Run verification test
    singleTest();

    return 0;
}
