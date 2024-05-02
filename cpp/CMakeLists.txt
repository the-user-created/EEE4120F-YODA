cmake_minimum_required(VERSION 3.27)
project(MD5Sequential)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(include)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Add executable
add_executable(md5_cpp ${SOURCES})
