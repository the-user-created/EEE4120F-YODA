cmake_minimum_required(VERSION 3.27)
project(MD5Parallel)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# OpenCL
find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})
include_directories(include)

file(GLOB SOURCES "src/*.cpp" "src/*.cl")

add_executable(md5_opencl ${SOURCES})
target_link_libraries(md5_opencl ${OpenCL_LIBRARIES})
