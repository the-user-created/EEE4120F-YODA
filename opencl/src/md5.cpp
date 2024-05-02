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
std::vector<double> runMD5Hashing(const std::string& message, size_t local_size, size_t _global_size) {
    int messageLength = message.length();
    std::vector<char> input(message.begin(), message.end());

    // Create a vector to store execution times
    std::vector<double> executionTimes;

    cl_mem input_buffer, output_buffer;

    // Initialize OpenCL Platform
    cl_uint platformCount;
    cl_platform_id* platforms;
    clGetPlatformIDs(5, nullptr, &platformCount);
    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * platformCount);
    clGetPlatformIDs(platformCount, platforms, nullptr);
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
    char* program_buffer = (char*)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&program_buffer, &program_size, &err);
    //clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
    err = clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        // The program failed to build, print the build log for debugging
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        char *log = new char[log_size];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, nullptr);
        std::cerr << "Build failed; error=" << err << ", log:\n" << log << std::endl;
        delete[] log;
        exit(1);
    }

    // Create the MD5 kernel
    cl_kernel kernel = clCreateKernel(program, "md5_hash", &err);

    // NOTE: This code is OpenCL version dependent so that it will work on older iMac's
    // before their transition to Metal graphics (at which point Apple depreciated support for OpenCL)
    // (Only OpenCL 1.x is supported on these devices)
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

    size_t max_work_group_size;
    clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, nullptr);

    printf("Max work group size: %zu\n", max_work_group_size);
    printf("Local work size: %zu\n", local_work_size);
    printf("Global work size: %zu\n", global_size);

    // Create input and output buffers
    input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(char) * messageLength, nullptr, &err);
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 16 * sizeof(char), nullptr, &err);  // MD5 outputs a 128-bit hash

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buffer);
    clSetKernelArg(kernel, 2, sizeof(int), &messageLength);

    // Write the input data to the input buffer
    err = clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0, sizeof(char) * messageLength, input.data(), 0, nullptr, nullptr);
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
    char* output = new char[16];
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, 16 * sizeof(char), output, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to read output array!\n";
        exit(1);
    }

    // Print the output
    for (int i = 0; i < 16; i++) {
        printf("%02x", (unsigned char)output[i]);
    }
    printf("\n");

    // Clean up
    delete[] output;

    // Deallocate resources
    clReleaseKernel(kernel);
    clReleaseMemObject(input_buffer);
    clReleaseMemObject(output_buffer);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);
    free(platforms);

    return executionTimes;
}

int main() {
    std::string message = "The quick brown fox jumps over the lazy dog";
    int messageLength = message.length();

    // Compute the number of 512-bit blocks in the message
    int numBlocks = (messageLength + 63) / 64;

    std::vector<double> times;
    size_t best_local_size = 0;
    double best_time = std::numeric_limits<double>::max();

    // Test local sizes from 1 to 256
    for (size_t local_size = 1; local_size <= 256; ++local_size) {
        if (numBlocks % local_size != 0) {
            continue;  // Skip if global_size is not a multiple of local_size
        }
        times = runMD5Hashing(message, local_size, numBlocks);
        std::cout << "Time: " << times[0] << "\n";
        double avg_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        if (avg_time < best_time) {
            best_time = avg_time;
            best_local_size = local_size;
        }
        std::cout << "Average time: " << avg_time << "\n";
    }

    std::cout << "Best local size: " << best_local_size << "\n";
    return 0;
}
