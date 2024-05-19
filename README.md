# YODA: MD5 Hashing Algorithm Implementation

This repository contains implementations of the MD5 hashing algorithm in three different forms: sequential in C++, parallelized with OpenCL, and on an FPGA using Verilog.
Implementations of the MD6 hashing algorithm are also included.

## Folder Structure

The project is organized into three main folders:
- `cpp`: This contains the sequential implementation of the MD5 algorithm in C++.
- `opencl`: This contains the version of the MD5 algorithm using OpenCL.
- `verilog`: This hosts the (planned) FPGA implementation of the MD5 algorithm in Verilog.
- `md6`: This contains the sequential and parallel implementation of the MD6 algorithm in C++.

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
git clone https://github.com/the-user-created/EEE4120F-YODA.git
cd EEE4120F-YODA
```

## Building and Running
The Makefiles in each of the folders are setup for easily building and running the code.
The instructions are identical for each of the implementations.

```bash
cd <implementation_folder>
make re
```

## TODOs
- [ ] Implement the FPGA version of the MD5 algorithm on Nexys A7 FPGA board.

## Authors
- David Young - [YNGDAV005@myuct.ac.za](mailto:YNGDAV005@myuct.ac.za)
- Caide Spriestersbach - [SPRCAI002@myuct.ac.za](mailto:SPRCAI002@myuct.ac.za)
- Dylan Trowsdale - [TRWDYL001@myuct.ac.za](mailto:TRWDYL001@myuct.ac.za)
