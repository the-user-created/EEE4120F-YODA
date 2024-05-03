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

// Function to run MD5 hashing and return execution times
std::vector<double> runMD5Hashing(const std::vector<char>& message, size_t local_size, size_t _global_size, bool printOutput = false) {
    // Get the length of the message
    int messageLength = message.size();

    // Create a vector to store execution times
    std::vector<double> executionTimes;

    cl_mem input_buffer, output_buffer;

    // Initialize OpenCL Platform
    cl_uint platformCount = 0;
    std::unique_ptr<cl_platform_id[]> platforms(new cl_platform_id[platformCount]);
    clGetPlatformIDs(platformCount, platforms.get(), nullptr);
    cl_platform_id platform = platforms[0];

    // Initialize OpenCL Device
    cl_device_id device;
    cl_int err;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    if (err == CL_DEVICE_NOT_FOUND) {
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, nullptr);
    }

    // Create Context
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);

    // Read and compile the kernel
    FILE* program_handle = fopen("bin/Kernel.cl", "r");
    fseek(program_handle, 0, SEEK_END);
    size_t program_size = ftell(program_handle);
    rewind(program_handle);
    std::unique_ptr<char[]> program_buffer(new char[program_size + 1]);
    program_buffer[program_size] = '\0';
    fread(program_buffer.get(), sizeof(char), program_size, program_handle);
    fclose(program_handle);

    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&program_buffer, &program_size, &err);
    err = clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        // The program failed to build, print the build log for debugging
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        std::unique_ptr<char[]> log(new char[log_size]);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log.get(), nullptr);
        std::cerr << "Build failed; error=" << err << ", log:\n" << log.get() << std::endl;
        exit(1);
    }

    // Create the MD5 kernel
    cl_kernel kernel = clCreateKernel(program, "md5_hash", &err);

    // Get the OpenCL version
    #if defined(CL_VERSION_2_0)
        const cl_queue_properties props[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };
            cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, props, &err);
    #else
        cl_command_queue queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    #endif

    // Set up data buffers
    size_t global_size = _global_size;
    size_t local_work_size = local_size;

    // Create input and output buffers
    input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(char) * messageLength, nullptr, &err);
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 16 * sizeof(char), nullptr, &err);  // MD5 outputs a 128-bit hash

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buffer);
    clSetKernelArg(kernel, 2, sizeof(int), &messageLength);

    // Write the input data to the input buffer
    err = clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0, sizeof(char) * messageLength, message.data(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to write to source array!\n";
        exit(1);
    }

    // Execute the kernel
    cl_event event;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global_size, &local_work_size, 0, nullptr, &event);
    if (err) {
        std::cerr << "Failed to execute kernel!\n";
        exit(1);
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
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, 16 * sizeof(char), output.get(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to read output array!\n";
        exit(1);
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
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

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
    // Open the file
    std::ifstream file("src/to_hash.txt");  // File containing the message to hash
    if (!file) {
        std::cerr << "Unable to open file to_hash.txt";
        return 1;  // return with error code 1
    }

    // Read the file content
    std::string message((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

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

    std::vector<double> times;
    size_t local_size = 1;  // Set local size to 1

    std::vector<double> exec_times;

    // Run MD5 hashing 100 times
    for (int i = 0; i < 100; ++i) {
        if (i == 99) {
            // Print the output for the last iteration
            exec_times = runMD5Hashing(paddedMessage, local_size, numBlocks, true);
        } else {
            exec_times = runMD5Hashing(paddedMessage, local_size, numBlocks);
        }

        // Add the padding time to each execution time
        for (auto& time : exec_times) {
            time += paddingTime;
        }

        times.insert(times.end(), exec_times.begin(), exec_times.end());
    }

    // Calculate the average time
    double avg_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();

    std::cout << "Average time: " << avg_time << "\n";

    return 0;
}
