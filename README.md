# Intel Paillier Cryptosystem Library
Intel Paillier Cryptosystem Library is an open-source library which provides accelerated performance of a partial homomorphic encryption (HE), named Paillier cryptosystem, by utilizing Intel® [Integrated Performance Primitives Cryptography](https://github.com/intel/ipp-crypto) technologies on Intel CPUs supporting the AVX512IFMA instructions. The library is written in modern standard C++ and provides the essential API for the Paillier cryptosystem scheme.

## Contents
- [Intel Paillier Cryptosystem Library](#intel-paillier-cryptosystem-library)
  - [Contents](#contents)
  - [Introduction](#introduction)
  - [Building the Library](#building-the-library)
    - [Requirements](#requirements)
    - [Dependencies](#dependencies)
    - [Instructions](#instructions)
  - [Testing and Benchmarking](#testing-and-benchmarking)
- [Standardization](#standardization)
- [Contributors](#contributors)

## Introduction
Paillier cryptosystem is a probabilistic asymmetric algorithm for public key cryptography and a partial homomorphic encryption scheme which allows two types of computation:
- addition of two ciphertexts
- addition and multiplication of a ciphertext by a plaintext number

As a public key encryption scheme, Paillier cryptosystem has three stages:

 - Generate public-private key pair
 - Encryption with public key
 - Decryption with private key

For increased security, typically the key length is at least 1024 bits, but recommendation is 2048 bits or larger. To handle such large size integers, conventional implementations of the Paillier cryptosystem utilizes the GNU Multiple Precision Arithmetic Library (GMP). The essential computation of the scheme relies on the modular exponentiation, and our library takes advantage of the multi-buffer modular exponentiation function (```mbx_exp_mb8```) of IPP-Crypto library, which is enabled in AVX512IFMA instruction sets supporting SKUs, such as Intel Icelake Xeon CPUs.

## Building the Library
### Requirements
The hardware requirement to use the library is AVX512IFMA instruction sets enabled CPUs, as listed in Intel codenames:
 - Intel Cannon Lake
 - Intel Ice Lake

Note: We are planning to add support for more SKUs.

As for the operating systems, ehe library has been tested and confirmed to work on Ubuntu 18.04, 20.04 and RHEL 8.0.

### Dependencies
Must have dependencies include:
```
cmake >=3.15.1
git
pthread
g++ >= 7.0 or clang >= 10.0
```

The following libraries are also required,
```
nasm>=2.15
OpenSSL>=1.1.0
```

For ```nasm```, please refer to the [Netwide Assembler](https://nasm.us/) for installation details.

On Ubuntu, ```OpenSSL``` can be installed by:
```bash
sudo apt update
sudo apt install libssl-dev
```
On RHEL, it needs to be built and installed from source as the static libraries are not installed with package managers. Please refer to [OpenSSL Project](https://github.com/openssl/openssl) for installation details for static libraries.

### Instructions
The library can be built using the following commands:
```bash
export IPCL_DIR=$(pwd)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

It is possible to pass additional options to enable more features. The following table contains the current CMake options, default values are in bold.

| CMake options           | Values    | Default | Comments                            |
|-------------------------|-----------|---------|-------------------------------------|
|`IPCL_TEST`              | ON/OFF    | ON      | unit-test                           |
|`IPCL_BENCHMARK`         | ON/OFF    | ON      | benchmark                           |
|`IPCL_ENABLE_OMP`        | ON/OFF    | ON      | enables OMP unit-test and benchmark |
|`IPCL_DOCS`              | ON/OFF    | OFF     | build doxygen documentation         |
|`IPCL_SHARED`            | ON/OFF    | ON      | build shared library                |

## Testing and Benchmarking
To run a set of unit tests via [Googletest](https://github.com/google/googletest), configure and build library with `-DIPCL_TEST=ON` (see [Instructions](#instructions)).
Then, run
```bash
cmake --build build --target unittest
```

For running benchmark via [Google Benchmark](https://github.com/google/benchmark), configure and build library with `-DIPCL_BENCHMARK=ON` (see [Instructions](#instructions)).
Then, run
```bash
cmake --build build --target benchmark
```
OpenMP unit-tests and benchmarks will automatically be applied if `-DIPCL_ENABLE_OMP=ON` is set.

The executables are located at `${IPCL_DIR}/build/test/unittest_ipcl` and `${IPCL_DIR}/build/benchmark/bench_ipcl`.

# Standardization
This library is in compliance with the homomorphic encryption standards [ISO/IEC 18033-6](https://www.iso.org/standard/67740.html).
The compliance test is included in the [unit-test](test/test_cryptography.cpp#L112-L256).

# Contributors
Main contributors to this project, sorted by alphabetical order of last name are:
  - [Xiaoran Fang](https://github.com/fangxiaoran)
  - [Hengrui Hu](https://github.com/hhr293)
  - [Xiaojun Huang](https://github.com/xhuan28)
  - [Hamish Hunt](https://github.com/hamishun)
  - [Sejun Kim](https://github.com/skmono) (lead)
  - [Bin Wang](https://github.com/bwang30)
  - [Pengfei Zhao](https://github.com/justalittlenoob)
