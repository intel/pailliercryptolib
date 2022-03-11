# Intel Homomorphic Encryption (HE) Acceleration Library for Quick Assist Technology (QAT)
Intel Homomorphic Encryption Acceleration Library for QAT (HEQAT Lib) is an open-source library which provides accelerated performance for homomorphic encryption (HE) math functions involving multi-precision numbers and modular arithmetic. This library is written in C99. 

## Contents
- [Intel Homomorphic Encryption Acceleration Library for QAT](#intel-homomorphic-encryption-library-for-qat)
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

This library is underconstruction and currently only offers acceleration of modular exponentiation of multi-precision numbers, i.e. large numbers whose precision range from 1024 to 8192 bits. Current stage of implementation supports modular exponentiation of big numbers encoded with OpenSSL's BIGNUM data type and IPP Crypto's BigNumber class. More details about the modes of operation and characteristics of the execution flow are described below:

 - Synchronous: It means that calls will be made to send modular exponentiation work requests to be offloaded to the accelerator and processed in the order they are issued.

 - Blocking: It means that the next buffered work request waits for completion of the processing of the most recent request offloaded to the accelerator, when processing must be currently in progress.

 - Batch Support: The internal buffer is set accommodate 16 request at a time so that the maximum batch size is 16. Therefore, only up to 16 requests can be exercised asynchronously from the application side, be it from a single `for loop` or static code block. Finally, in order to collect the requests, a call to the `get` function that waits for completion of the requests must be done. 

 - Single-Threaded: It is currently safer to use single-threaded, although multiple threading is partially supported (try it at your own risk). Multi-threading can only be supported so long as the internal buffer can fill all the requests submitted by multiple threads, otherwise it will hang (this feature will become reliable in later versions).

 - Single instance: The library is configure to use only 1 single QAT endpoint at any time at the creation of the QAT runtime context.

## Building the Library

```
export ICP_ROOT=$HOME/QAT
cmake -S . -B build -DHE_QAT_MISC=OFF
cmake --build build
cmake --install build
```

`HE_QAT_MISC` enables IPP Crypto. If you want to enable that, follow the build instructions below:

```
git clone https://github.com/intel/ipp-crypto.git
cd ipp-crypto
CC=gcc CXX=g++ cmake CMakeLists.txt -B_build -DARCH=intel64 -DMERGED_BLD:BOOL=on -DCMAKE_INSTALL_PREFIX=/opt/ipp-crypto -DOPENSSL_INCLUDE_DIR=/opt/openssl/include -DOPENSSL_LIBRARIES=/opt/openssl/lib -DOPENSSL_ROOT_DIR=/opt/openssl -DCMAKE_ASM_NASM_COMPILER=/opt/nasm-2.15/bin/nasm
cmake --build _build -j
sudo cmake --install _build
```

### Running Examples

```
./build/examples/test_context
``` 

```
./build/examples/test_bnModExpPerformOp
``` 

```
./build/examples/test_bnConversion
``` 

### Requirements
The hardware requirement to use the library is the following:
 - Intel Sapphire Rapids
 - Intel C62XX acceleration card

As for the operating systems, the library has been tested and confirmed to work on Ubuntu 20.04.

### Dependencies

Required dependencies include:
```
cmake >=3.15.1
git
pthread
gcc >= 9.1
qatlib
ippcrypto
```

