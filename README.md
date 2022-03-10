# Intel Homomorphic Encryption (HE) Acceleration Library for Quick Assist Technology (QAT)
Intel Homomorphic Encryption Acceleration Library for QAT (HEQAT Lib) is an open-source library which provides accelerated performance for homomorphic encryption (HE) math functions involving multi-precision numbers and modular arithmetic. This library is written in C99. 

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

This library is underconstruction and currently only offers acceleration of modular exponentiation of multi-precision numbers, ranging from 1024 to 8192-bit numbers. Current stage of implementation provides the big number modular exponentiation featuring OpenSSL BIGNUM data type and has the following characteristics:
 - Synchronous: It means that calls will place a work request to be offloaded to the accelerator and processed in the order they are issued.
 - Blocking: It means that the next buffered work request waits for completion of processing of the most recently request offloaded to the accelerator, when processing must be currently in progress.
 - Batch Support: The internal buffer is set accommodate 16 request at once so that the maximum batch size is 16. Therefore, only up to 16 requests can exercised asynchronously from the application for any single `for loop` or code block along with a call to the `get` function that waits for completion of the requests. 
 - Single-Threaded: It is currently safer to use single-threaded, although multiple threading is partially supported (try it at your own risk). Multi-threading can only be supported so long as the internal buffer can fill all the requests submitted by multiple threads, otherwise it will hang (this feature will become reliable in later versions).
 - Single instance: The library is configure to use only 1 single QAT endpoint at any time at the creation of the QAT runtime context.

## Building the Library
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
```

