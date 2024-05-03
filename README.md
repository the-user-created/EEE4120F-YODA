# MD5 Hashing Algorithm Implementation

This repository contains implementations of the MD5 hashing algorithm in three different forms: sequential in C++, parallelized with OpenCL, and on an FPGA using Verilog.

## Branch Structure

The project is organized into three main branches:
- `cpp`: This branch contains the sequential implementation of the MD5 algorithm in C++.
- `opencl`: This branch includes the parallelized version of the MD5 algorithm using OpenCL.
- `verilog`: This branch hosts the FPGA implementation of the MD5 algorithm in Verilog.

## Getting Started

Below are the instructions to set up each environment for development and testing:

### Prerequisites

- C++ compiler (e.g., GCC, Clang)
- OpenCL SDK (for example, from Intel, AMD, or Nvidia)
- Icarus Verilog for FPGA simulation
- Git to clone and manage branches

### Cloning the Repository

To clone this repository and switch to a branch, use the following commands:

```bash
git clone https://github.com/<your-username>/EEE4120F-YODA.git
cd EEE4120F-YODA
git checkout <branch-name>  # Replace <branch-name> with cpp, opencl, or verilog
```

## Building and Running

### C++ Implementation
```bash
cd cpp
make
make run
```

### OpenCL Implementation
```bash
cd opencl
make
make run
```

### Verilog Implementation
```bash
cd verilog
make
make run
```

## TODOs
- [ ] Optimize the OpenCL implementation
- [ ] Implement the FPGA version of the MD5 algorithm
- [ ] Test various C compiler flags (can comment on this in the report)

## Authors
- David Young - [YNGDAV005@myuct.ac.za](mailto:YNGDAV005@myuct.ac.za)
- Caide Spriestersbach - [SPRCAI002@myuct.ac.za](mailto:SPRCAI002@myuct.ac.za)
- Dylan Trowsdale - [TRWDYL001@myuct.ac.za](mailto:TRWDYL001@myuct.ac.za)
