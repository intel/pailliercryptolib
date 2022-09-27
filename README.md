# Intel Paillier Cryptosystem Library
Intel Paillier Cryptosystem Library is an open-source library which provides accelerated performance of a partial homomorphic encryption (HE), named Paillier cryptosystem, by utilizing Intel® [Integrated Performance Primitives Cryptography](https://github.com/intel/ipp-crypto) technologies on Intel CPUs supporting the AVX512IFMA instructions. The library is written in modern standard C++ and provides the essential API for the Paillier cryptosystem scheme.

## Contents
- [Intel Paillier Cryptosystem Library](#intel-paillier-cryptosystem-library)
  - [Contents](#contents)
  - [Introduction](#introduction)
    - [Performance](#performance)
  - [Building the Library](#building-the-library)
    - [Prerequisites](#prerequisites)
    - [Dependencies](#dependencies)
    - [Downloading](#downloading)
    - [Instructions](#instructions)
  - [Testing and Benchmarking](#testing-and-benchmarking)
- [Python Extension](#python-extension)
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

### Performance
To be added after P2CA review
## Building the Library
### Prerequisites
For best performance, especially due to the multi-buffer modular exponentiation function, the library is to be used on AVX512IFMA enabled systems, as listed below in Intel CPU codenames:
 - Intel Cannon Lake
 - Intel Ice Lake

The library can be built and used without AVX512IFMA, as if the instruction set is not detected on the system, it will automatically switch to non multi-buffer modular exponentiation.

The following operating systems have been tested and deemed to be fully functional.
  - Ubuntu 18.04 and higher
  - Red Hat Enterprise Linux 8.1 and higher

We will keep working on adding more supported operating systems.
### Dependencies
Must have dependencies include:
```
cmake >= 3.15.1
git
pthread
g++ >= 7.0 or clang >= 10.0
```

The following libraries and tools are also required,
```
nasm >= 2.15
OpenSSL >= 1.1.0
numa >= 2.0.12
```

For ```nasm```, please refer to the [Netwide Assembler](https://nasm.us/) for installation details.

On Ubuntu, ```OpenSSL``` and ```numa``` can be installed with:
```bash
sudo apt update
sudo apt install libssl-dev
sudo apt install libnuma-dev
```
For RHEL, ```OpenSSL``` needs to be built and installed from source as the static libraries are missing when installed through the package managers. Please refer to [OpenSSL Project](https://github.com/openssl/openssl) for installation details for static libraries. ```numa``` can be installed with:
```
sudo yum install numactl-devel
```
### Downloading
Pull source code from git repository with all submodules. There are two ways to do it:
- Clone source code with all submodules at once
```
git clone --recursive https://github.com/intel/pailliercryptolib.git
cd pailliercryptolib
```
- Clone source code, then pull submodules
```
git clone https://github.com/intel/pailliercryptolib.git
cd pailliercryptolib
git submodule update --init
```
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
|`IPCL_ENABLE_QAT`        | ON/OFF    | OFF     | enables QAT functionalities         |
|`IPCL_ENABLE_OMP`        | ON/OFF    | ON      | enables OpenMP functionalities      |
|`IPCL_THREAD_COUNT`      | Integer   | OFF     | The max number of threads           |
|`IPCL_DOCS`              | ON/OFF    | OFF     | build doxygen documentation         |
|`IPCL_SHARED`            | ON/OFF    | ON      | build shared library                |

## Compiling for QAT

Install QAT software stack following the [instructions from the HE QAT Lib](https://github.com/intel-sandbox/libraries.security.cryptography.homomorphic-encryption.glade.project-destiny/tree/development#installing-qat-software-stack). The current QAT support is not multithreading safe; therefore, `IPCL_ENABLE_OMP` must be set to `OFF`.

```bash
export IPCL_DIR=$(pwd)
export ICP_ROOT=$HOME/QAT
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DIPCL_ENABLE_QAT=ON -DIPCL_ENABLE_OMP=OFF
cmake --build build -j
```

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
Setting the CMake flag ```-DIPCL_ENABLE_OMP=ON``` during configuration will use OpenMP for acceleration. Setting the value of `-DIPCL_THREAD_COUNT` will limit the maximum number of threads used by OpenMP (If set to OFF or 0, its actual value will be determined at run time).

The executables are located at `${IPCL_DIR}/build/test/unittest_ipcl` and `${IPCL_DIR}/build/benchmark/bench_ipcl`.

# Python Extension
Alongside the Intel Paillier Cryptosystem Library, we provide a Python extension package utilizing this library as a backend. For installation and usage detail, refer to [Intel Paillier Cryptosystem Library - Python](https://github.com/intel-sandbox/libraries.security.cryptography.homomorphic-encryption.glade.pailliercryptolib-python).

# Standardization
This library is in compliance with the homomorphic encryption standards [ISO/IEC 18033-6](https://www.iso.org/standard/67740.html).
The compliance test is included in the [unit-test](https://github.com/intel-sandbox/libraries.security.cryptography.homomorphic-encryption.glade.pailliercryptolib/blob/main/test/test_cryptography.cpp#L112-L256).

# Contributors
Main contributors to this project, sorted by alphabetical order of last name are:
  - [Xiaoran Fang](https://github.com/fangxiaoran)
  - [Hengrui Hu](https://github.com/hhr293)
  - [Xiaojun Huang](https://github.com/xhuan28)
  - [Hamish Hunt](https://github.com/hamishun)
  - [Sejun Kim](https://github.com/skmono) (lead)
  - [Bin Wang](https://github.com/bwang30)
  - [Pengfei Zhao](https://github.com/justalittlenoob)
