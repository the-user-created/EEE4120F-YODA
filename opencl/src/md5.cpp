//
// Created by David Young on 2024/05/02.
//

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include<CL/cl.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>

#include "OpenCLError.h"

// Class to manage OpenCL resources
class OpenCLResources {
private:
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;

public:
    OpenCLResources() {
        cl_int err;

        // Initialize OpenCL Platform
        cl_uint platformCount = 0;
        clGetPlatformIDs(0, nullptr, &platformCount);
        std::unique_ptr<cl_platform_id[]> platforms(new cl_platform_id[platformCount]);
        clGetPlatformIDs(platformCount, platforms.get(), nullptr);
        platform = platforms[0];

        // Initialize OpenCL Device
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
        if (err == CL_DEVICE_NOT_FOUND) {
            err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, nullptr);
        }
        if (err != CL_SUCCESS) {
            throw OpenCLError("Failed to initialize OpenCL device");
        }

        // Create Context
        context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);

        // Create Command Queue
#if defined(CL_VERSION_2_0)
        const cl_queue_properties props[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };
        queue = clCreateCommandQueueWithProperties(context, device, props, &err);
#else
        queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
#endif

        // Read and compile the kernel
        FILE* program_handle = fopen("bin/Kernel.cl", "r");
        fseek(program_handle, 0, SEEK_END);
        size_t program_size = ftell(program_handle);
        rewind(program_handle);
        std::unique_ptr<char[]> program_buffer(new char[program_size + 1]);
        program_buffer[program_size] = '\0';
        fread(program_buffer.get(), sizeof(char), program_size, program_handle);
        fclose(program_handle);

        program = clCreateProgramWithSource(context, 1, (const char**)&program_buffer, &program_size, nullptr);
        err = clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
        if (err != CL_SUCCESS) {
            // The program failed to build, print the build log for debugging
            size_t log_size;
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
            std::unique_ptr<char[]> log(new char[log_size]);
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log.get(), nullptr);
            throw OpenCLError(std::string("Build failed; error=") + std::to_string(err) + ", log:\n" + log.get());
        }
    }

    ~OpenCLResources() {
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
    }

    cl_platform_id getPlatform() const { return platform; }
    cl_device_id getDevice() const { return device; }
    cl_context getContext() const { return context; }
    cl_command_queue getQueue() const { return queue; }
    cl_program getProgram() const { return program; }
};

// Function to run MD5 hashing and return execution times
double runMD5Hashing(OpenCLResources& resources, const std::vector<char>& message, size_t local_size, size_t numBlocks, bool printOutput = false) {
    // Get the length of the message
    int messageLength = message.size();

    // Create a vector to store execution times
    std::vector<double> executionTimes;

    cl_mem input_buffer, output_buffer;

    // Use the compiled program
    cl_program program = resources.getProgram();

    cl_int err;

    // Create the MD5 kernel
    cl_kernel kernel = clCreateKernel(program, "md5_hash", &err);

    // Set up data buffers
    size_t global_size = numBlocks;
    size_t local_work_size = local_size;

    // Create input and output buffers
    input_buffer = clCreateBuffer(resources.getContext(), CL_MEM_READ_ONLY, sizeof(char) * messageLength, nullptr, &err);
    output_buffer = clCreateBuffer(resources.getContext(), CL_MEM_WRITE_ONLY, 16 * sizeof(char), nullptr, &err);  // MD5 outputs a 128-bit hash

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buffer);
    clSetKernelArg(kernel, 2, sizeof(int), &messageLength);

    // Write the input data to the input buffer
    err = clEnqueueWriteBuffer(resources.getQueue(), input_buffer, CL_TRUE, 0, sizeof(char) * messageLength, message.data(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        throw OpenCLError("Failed to write to source array");
    }

    // Execute the kernel
    cl_event event;
    err = clEnqueueNDRangeKernel(resources.getQueue(), kernel, 1, nullptr, &global_size, &local_work_size, 0, nullptr, &event);
    if (err) {
        throw OpenCLError("Failed to execute kernel");
    }

    // Wait for the kernel to finish executing
    clWaitForEvents(1, &event);

    // Get the execution time
    cl_ulong startTime, endTime;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, nullptr);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, nullptr);
    double executionTime = (endTime - startTime) * 1e-9;  // Convert from nanoseconds to seconds

    // Read the output buffer back to the host
    std::unique_ptr<char[]> output(new char[16]);
    err = clEnqueueReadBuffer(resources.getQueue(), output_buffer, CL_TRUE, 0, 16 * sizeof(char), output.get(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        throw OpenCLError("Failed to read output array");
    }

    // Print the output
    if (printOutput) {
        for (int i = 0; i < 16; i++) {
            printf("%02x", output[i] & 0xFF);
        }
        std::cout << "\n";
    }

    // Deallocate resources
    clReleaseMemObject(input_buffer);
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);

    return executionTime;
}

// Function to pad the message to a multiple of 512 bits
std::vector<char> padMessage(const std::string& message) {
    std::vector<char> paddedMessage(message.begin(), message.end());

    // Add the bit '1' to the message
    paddedMessage.push_back(0x80);

    // Add zeros until the length of the message in bits is 448 (mod 512)
    while ((paddedMessage.size() * 8) % 512 != 448) {
        paddedMessage.push_back(0);
    }

    // Append the original length (before padding) as a 64-bit little-endian integer
    uint64_t originalLength = message.size() * 8;  // Convert to bits
    for (int i = 0; i < 8; i++) {
        paddedMessage.push_back((originalLength >> (i * 8)) & 0xFF);
    }

    return paddedMessage;
}


void runTests() {
    int executions = 3;
    std::vector<double> times(executions, 0); // Vector to store all execution times

    // Create an instance of OpenCLResources
    OpenCLResources resources;

    // Loop over different input sizes
    for (unsigned long long inputSize = 0; inputSize < pow(2, 20); inputSize += 4 * ceil(pow(2, 20) / 200)) {
        // Generate input string of the required size
        std::string message(inputSize, 'a'); // Fill the string with 'a'

        std::cout << "Running " << executions << " executions of MD5 hashing on input size " << inputSize << "\n";

        // Start the timer
        auto start = std::chrono::high_resolution_clock::now();

        // Pad the message
        std::vector<char> paddedMessage = padMessage(message);

        // Compute the number of 512-bit blocks in the message
        int numBlocks = paddedMessage.size() / 64;

        // Stop the timer
        auto stop = std::chrono::high_resolution_clock::now();

        // Compute the time it took to pad the message
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        double paddingTime = duration.count() * 1e-6;  // Convert from microseconds to seconds

        for (int i = 0; i < executions; ++i) {
            double exec_time = runMD5Hashing(resources, paddedMessage, 1, numBlocks);

            // Add the padding time to the execution time
            times[i] = exec_time + paddingTime;
        }

        // Write the execution times to a CSV file
        std::ofstream outputFile("execution_times_3.csv", std::ios_base::app); // Append to the file
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
    // Create an instance of OpenCLResources
    OpenCLResources resources;
    std::string message = "The quick brown fox jumps over the lazy dog";

    // Start the timer
    auto start = std::chrono::high_resolution_clock::now();

    // Pad the message
    std::vector<char> paddedMessage = padMessage(message);

    // Compute the number of 512-bit blocks in the message
    int numBlocks = paddedMessage.size() / 64;

    // Stop the timer
    auto stop = std::chrono::high_resolution_clock::now();

    // Compute the time it took to pad the message
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    double paddingTime = duration.count() * 1e-6;  // Convert from microseconds to seconds

    std::cout << "MD5 hash of '" << message << "': \n";
    double exec_time = runMD5Hashing(resources, paddedMessage, 1, numBlocks, true);
    // Expected hash: 9e107d9d372bb6826bd81d3542a419d6

    std::cout << "Execution time: " << exec_time + paddingTime << " seconds\n";
}


int main() {
    // Run a single test
    singleTest();

    // Run multiple tests - note that this will take a long time to run
    // runTests();

    return 0;
}
