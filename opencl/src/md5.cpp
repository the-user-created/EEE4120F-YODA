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
std::vector<double> runMD5Hashing(OpenCLResources& resources, const std::vector<char>& message, size_t local_size, size_t numBlocks, bool printOutput = false) {
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

    // Store the execution time
    executionTimes.push_back(executionTime);

    // Read the output buffer back to the host
    std::unique_ptr<char[]> output(new char[16]);
    err = clEnqueueReadBuffer(resources.getQueue(), output_buffer, CL_TRUE, 0, 16 * sizeof(char), output.get(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        throw OpenCLError("Failed to read output array");
    }

    // Print the output
    if (printOutput) {
        std::cout << "MD5 Hash: ";
        for (int i = 0; i < 16; i++) {
            printf("%02x", output[i] & 0xFF);
        }
        std::cout << "\n";
    }

    // Deallocate resources
    clReleaseMemObject(input_buffer);
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);

    return executionTimes;
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


int main() {
    // Message to hash
    std::string message = "The quick brown fox jumps over the lazy dog";

    // Start the timer
    auto start = std::chrono::high_resolution_clock::now();

    // Pad the message
    std::vector<char> paddedMessage = padMessage(message);

    // Compute the number of 512-bit blocks in the message
    int numBlocks = paddedMessage.size() / 64;

    // print numblocks and padded message size
    std::cout << "Number of blocks: " << numBlocks << "\n";
    std::cout << "Padded message size: " << paddedMessage.size() << "\n";

    // Stop the timer
    auto stop = std::chrono::high_resolution_clock::now();

    // Compute the time it took to pad the message
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    double paddingTime = duration.count() * 1e-6;  // Convert from microseconds to seconds

    std::vector<double> times;
    size_t local_size = 1;

    // Local work size determines how many work items (threads) are in a work group
    // A work group executes on a single compute unit.
    // The work items in a work group can communicate with each other using local memory.
    // The local work size has these constraints:
    // - The total number of work items in a work group must be less than or equal to the maximum work group size. (CL_DEVICE_MAX_WORK_GROUP_SIZE)
    // - The local size must be evenly divisible by the global size.
    // - The local size in each dimension must be less than or equal to the maximum work-item sizes (CL_DEVICE_MAX_WORK_ITEM_SIZES)
    // - The amount of local memory used by the kernel must be less than or equal to the local memory size. (CL_DEVICE_LOCAL_MEM_SIZE)
    //
    // Optimal local size is often a multiple of the warp size (or wavefront size) of the GPU.

    // TODO: local/global size determination

    // Create an instance of OpenCLResources
    OpenCLResources resources;

    std::vector<double> exec_times;

    // Open a file in write mode.
    std::ofstream outfile;
    outfile.open("execution_times.csv");
    outfile << "Num,Time\n"; // Write the headers

    try {
        // Run MD5 hashing 100 times
        for (int i = 0; i < 1000; ++i) {
            if (i == 999) {
                // Print the output for the last iteration
                exec_times = runMD5Hashing(resources, paddedMessage, local_size, numBlocks, true);
            } else {
                exec_times = runMD5Hashing(resources, paddedMessage, local_size, numBlocks);
            }

            // Add the padding time to each execution time
            for (auto &time: exec_times) {
                time += paddingTime;
            }

            times.insert(times.end(), exec_times.begin(), exec_times.end());

            // Write the execution time to the CSV file
            outfile << i+1 << "," << exec_times[0] << "\n";
        }
    } catch (const OpenCLError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    outfile.close(); // Close the file

    // Calculate the average time
    double avg_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();

    // Calculate the minimum and maximum time
    double min_time = *std::min_element(times.begin(), times.end());
    double max_time = *std::max_element(times.begin(), times.end());

    // Calculate the standard deviation
    double sum_deviation = std::accumulate(times.begin(), times.end(), 0.0, [avg_time](double a, double b) { return a + pow(b - avg_time, 2); });
    double stddev_time = sqrt(sum_deviation / times.size());

    std::cout << "Average time: " << avg_time << " s\n";
    std::cout << "Minimum time: " << min_time << " s\n";
    std::cout << "Maximum time: " << max_time << " s\n";
    std::cout << "Standard deviation: " << stddev_time << " s\n";

    return 0;
}
