# Intel Paillier Cryptosystem Library
Intel Paillier Cryptosystem Library is an open-source library which provides accelerated performance of a partial homomorphic encryption (HE), named Paillier cryptosystem, by utilizing Intel® [IPP-Crypto](https://github.com/intel/ipp-crypto) technologies on Intel CPUs supporting the AVX512IFMA instructions. The library is written in modern standard C++ and provides the essential API for the Paillier cryptosystem scheme.

## Contents
- [Paillier Homomorphic Encryption library with Intel ipp-crypto](#paillier-homomorphic-encryption-library-with-intel-ipp-crypto)
  - [Contents](#content)
  - [Introduction](#introduction)
  - [Building the Library](#building-the-library)
    - [Dependencies](#dependencies)
    - [Instructions](#instructions)
  - [Testing and Benchmarking](#testing-and-benchmarking)
- [Contributors](#contributors)

## Introduction
adding intro

## Building the Library
### Dependencies
The library has been tested on Ubuntu 18.04 and 20.04

Must have dependencies include:
```
cmake >=3.15.1
git
pthread
g++ >= 7.0 or clang >= 10.0
python >= 3.8
```

The following libraries are also required,
```
nasm>=2.15
OpenSSL
```
which can be installed by:
```bash
sudo apt update
sudo apt install libssl-dev
```
For ```nasm```, please refer to the [Netwide Assembler webpage](https://nasm.us/) for installation details.

### Instructions
The library can be built using the following commands:
```bash
export IPCL_DIR=$(pwd)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

It is possible to pass additional options to enable more features. The following table contains the current CMake options, default values are in bold.

| CMake options           | Values    | Default | Comments                     |
|-------------------------|-----------|---------|------------------------------|
|`IPCL_TEST`              | ON/OFF    | ON      | unit-test                    |
|`IPCL_TEST_OMP`          | ON/OFF    | ON      | unit-test w/ OpenMP          |
|`IPCL_BENCHMARK`         | ON/OFF    | ON      | benchmark                    |
|`IPCL_DOCS`              | ON/OFF    | OFF     | build doxygen documentation  |
|`IPCL_SHARED`            | ON/OFF    | ON      | build shared library         |

## Testing and Benchmarking
To run a set of unit tests via [Googletest](https://github.com/google/googletest), configure and build library with `-DIPCL_TEST=ON` (see [Instructions](#instructions)).
Then, run
```bash
cmake --build build --target unittest
```
For OpenMP testing, configure and build with `-DIPCL_TEST_OMP=ON`, and run
```bash
cmake --build build --target unittest_omp
```

For running benchmark via [Google Benchmark](https://github.com/google/benchmark), configure and build library with `-DIPCL_BENCHMARK=ON` (see [Instructions](#instructions)).
Then, run
```bash
cmake --build build --target benchmark
```
OpenMP benchmarks will automatically be applied if `-DIPCL_TEST_OMP=ON` is set.

The unit-test executable itself is located at `build/test/unit-test`, `build/test/unit-test_omp` and `build/benchmark/bench_ipp_paillier`.

# Contributors
Main contributors to this project, sorted by alphabetical order of last name are:
  - [Xiaoran Fang](https://github.com/fangxiaoran)
  - [Hamish Hunt](https://github.com/hamishun)
  - [Sejun Kim](https://github.com/skmono) (lead)
  - [Bin Wang](https://github.com/bwang30)
