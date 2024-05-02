//
// Created by David Young on 2024/05/02.
//

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include<CL/cl.h>
#endif
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
