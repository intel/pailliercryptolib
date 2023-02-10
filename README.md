# Intel Paillier Cryptosystem Library
TEST
Intel Paillier Cryptosystem Library is an open-source library which provides accelerated performance of a partial homomorphic encryption (HE), named Paillier cryptosystem, by utilizing Intel® [Integrated Performance Primitives Cryptography](https://github.com/intel/ipp-crypto) technologies on Intel CPUs supporting the AVX512IFMA instructions and Intel® [Quickassist Technology](https://01.org/intel-quickassist-technology). The library is written in modern standard C++ and provides the essential API for the Paillier cryptosystem scheme. Intel Paillier Cryptosystem Library is certified for ISO compliance.

## Contents
- [Intel Paillier Cryptosystem Library](#intel-paillier-cryptosystem-library)
  - [Contents](#contents)
  - [Introduction](#introduction)
  - [Building the Library](#building-the-library)
    - [Prerequisites](#prerequisites)
    - [Dependencies](#dependencies)
    - [Instructions](#instructions)
    - [Installing and Using Example](#installing-and-using-example)
  - [Compiling for QAT](#compiling-for-qat)
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

For increased security, typically the key length is at least 1024 bits, but recommendation is 2048 bits or larger. To handle such large size integers, conventional implementations of the Paillier cryptosystem utilizes the GNU Multiple Precision Arithmetic Library (GMP). The essential computation of the scheme relies on the modular exponentiation, and our library takes advantage of two Intel features - the multi-buffer modular exponentiation function (```mbx_exp_mb8```) of IPP-Crypto library, which is enabled in AVX512IFMA instruction sets supporting SKUs, such as Intel Icelake/Sapphire Rapid Xeon® scalable processors and the modular exponentiation operation (```cpaCyLnModExp```) of Quickassist Technology library for QAT devices.

## Building the Library
### Prerequisites
For best performance, especially due to the multi-buffer modular exponentiation function, the library is to be used on AVX512IFMA enabled systems, as listed below in Intel CPU codenames:
 - Intel Cannon Lake
 - Intel Ice Lake
 - Intel Sapphire Rapids

The library can be built and used without AVX512IFMA and/or QAT, if the features are not supported. But for better performance, it is recommended to use the library on Intel Xeon® scalable processors - Ice Lake-SP or Sapphire Rapids-SP Xeon CPUs while fully utilizing the features.

The following operating systems have been tested and deemed to be fully functional.
  - Ubuntu 18.04 and higher
  - Red Hat Enterprise Linux 8.1 and higher
  - CentOS Stream

We will keep working on adding more supported operating systems.
### Dependencies
Must have dependencies include:
```
cmake >= 3.15.1
git
pthread
Intel C++ Compiler Classic 2021.3 for Linux* OS
Intel oneAPI DPC++/C++ Compiler for Linux* OS >= 2021.3
g++ >= 8.0
clang >= 10.0
```

The following libraries and tools are also required,
```
nasm >= 2.15
OpenSSL >= 1.1.0
```

```OpenSSL``` can be installed with:
```bash
# Ubuntu
sudo apt install libssl-dev
# Fedora (RHEL 8, Centos)
sudo dnf install openssl-devel
```

In order to install ```nasm```, please refer to the [Netwide Assembler webpage](https://nasm.us/) for download and installation details.

### Instructions
The library can be built using the following commands:
```bash
export IPCL_ROOT=$(pwd)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

It is possible to pass additional options to enable more features. The following table contains the current CMake options, default values are in bold.

| CMake options            | Values    | Default | Comments                            |
|--------------------------|-----------|---------|-------------------------------------|
|`IPCL_TEST`               | ON/OFF    | ON      | unit-test                           |
|`IPCL_BENCHMARK`          | ON/OFF    | ON      | benchmark                           |
|`IPCL_ENABLE_QAT`         | ON/OFF    | OFF     | enables QAT functionalities         |
|`IPCL_ENABLE_OMP`         | ON/OFF    | ON      | enables OpenMP functionalities      |
|`IPCL_THREAD_COUNT`       | Integer   | OFF     | explicitly set max number of threads|
|`IPCL_DOCS`               | ON/OFF    | OFF     | build doxygen documentation         |
|`IPCL_SHARED`             | ON/OFF    | ON      | build shared library                |
|`IPCL_DETECT_CPU_RUNTIME` | ON/OFF    | OFF     | detects CPU supported instructions (AVX512IFMA, rdseed, rdrand) during runtime |

If ```IPCL_DETECT_CPU_RUNTIME``` flag is ```ON```, it will determine whether the system supports the AVX512IFMA instructions on runtime. It is still possible to disable IFMA exclusive feature (multi-buffer modular exponentiation) during runtime by setting up the environment variable ```IPCL_DISABLE_AVX512IFMA=1```.

### Installing and Using Example
For installing and using the library externally, see [example/README.md](./example/README.md).

## Compiling for QAT

Install QAT software stack following the [Building the HE QAT Library](./module/heqat/README.md#building-the-library).

```bash
export IPCL_DIR=$(pwd)
export ICP_ROOT=$HOME/QAT
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DIPCL_ENABLE_QAT=ON
cmake --build build -j
```
For more details, please refer to the [HEQAT Readme](./module/heqat/README.md).

## Testing and Benchmarking
To run a set of unit tests via [GoogleTest](https://github.com/google/googletest), configure and build library with `-DIPCL_TEST=ON` (see [Instructions](#instructions)).
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

The executables are located at `${IPCL_ROOT}/build/test/unittest_ipcl` and `${IPCL_ROOT}/build/benchmark/bench_ipcl`.

# Python Extension
Alongside the Intel Paillier Cryptosystem Library, we provide a Python extension package utilizing this library as a backend. For installation and usage detail, refer to [Intel Paillier Cryptosystem Library - Python](https://github.com/intel/pailliercryptolib_python).

# Standardization
This library is certified for ISO compliance with the homomorphic encryption standards [ISO/IEC 18033-6](https://www.iso.org/standard/67740.html) by [Dekra](https://www.dekra.com).
# Contributors
Main contributors to this project, sorted by alphabetical order of last name are:
  - [Flavio Bergamaschi](https://www.linkedin.com/in/flavio-bergamaschi)
  - [Xiaoran Fang](https://github.com/fangxiaoran)
  - [Hengrui Hu](https://github.com/hhr293)
  - [Xiaojun Huang](https://github.com/xhuan28)
  - [Hamish Hunt](https://www.linkedin.com/in/hamish-hunt)
  - [Jingyi Jin](https://www.linkedin.com/in/jingyi-jin-655735)
  - [Sejun Kim](https://www.linkedin.com/in/sejun-kim-2b1b4866) (lead)
  - [Fillipe D.M. de Souza](https://www.linkedin.com/in/fillipe-d-m-de-souza-a8281820)
  - [Bin Wang](https://github.com/bwang30)
  - [Pengfei Zhao](https://github.com/justalittlenoob)
