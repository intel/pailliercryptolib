# Intel Homomorphic Encryption (HE) Acceleration Library for Quick Assist Technology (QAT)
Intel Homomorphic Encryption Acceleration Library for QAT (HE QAT Lib) is an open-source library which provides accelerated performance for homomorphic encryption (HE) math functions involving multi-precision numbers and modular arithmetic. This library is written in C99. 

## Contents
- [Intel Homomorphic Encryption Acceleration Library for QAT](#intel-homomorphic-encryption-library-for-qat)
  - [Contents](#contents)
  - [Introduction](#introduction)
  - [Building the Library](#building-the-library)
    - [Requirements](#requirements)
    - [Dependencies](#dependencies)
    - [Instructions](#instructions)
  - [Testing and Benchmarking](#testing-and-benchmarking)
<!-- - [Standardization](#standardization) -->
- [Contributors](#contributors)

## Introduction

This library is underconstruction and currently only offers acceleration of modular exponentiation of multi-precision numbers, i.e. large numbers whose precision range from 1024 to 8192 bits. Current stage of implementation supports modular exponentiation of big numbers encoded with OpenSSL's BIGNUM data type and IPP Crypto's BigNumber class. More details about the modes of operation and characteristics of the execution flow are described below:

 - Synchronous: It means that calls will be made to send modular exponentiation work requests to be offloaded to the accelerator and processed in the order they are issued.
 
 - Asynchronous: It means that multiple concurrent modular exponentiation work requests can be offloaded to the accelerator and processed not necessarily in the order they are issued from the host side.

 - Blocking: It means that the next buffered work request waits for completion of the processing of the most recent request offloaded to the accelerator, when processing must be currently in progress.

 - Batch Support: The internal buffer is set accommodate 256 requests at a time so that the maximum batch size is 256. Therefore, only up to 256 requests can be exercised asynchronously from the application side, be it from a single `for loop` or static code block. Finally, in order to collect the requests, a call to the `getBnModExpRequest()` function must be performed to wait for completion of all submitted asynchronous requests. 

 - Single-Threaded: It is currently desinged (and safer) to use it with a single-threaded applications, although multithreading is partially supported (try it at your own risk). Multithreading support is limited to work under restrictive conditions, namely, the total number of incoming requests at any point in time from multiple threads does not exceed the size of the internal buffer from which work requests are taken to be offloaded to QAT devices. Effective multithreading support will relax this condition by relying on a separate buffer that admits outstanding work requests. 
<!-- can only be supported so long as the internal buffer can fill all the requests submitted by multiple threads, otherwise it will hang (this feature will become reliable in later versions).-->

 - Multiple Instances/Devices: The library accesses all logical instances from all visible and configured QAT endpoints at the creation of the QAT runtime context. Therefore, if 8 QAT endpoints are available, it will attempt to use them all, including all the total number of logical instances configured per process. 

>> _**Note**_: Current implementation does not verify if the instance/endpoint has the capabilities needed by the library. For example, the library needs access to the _asym_ capabilities like `CyLnModExp`, therefore it the configuration file of an endpoint happens to be configured not offer it, the application will exit with an error at some point during execution.

## Building the HE QAT Library

### Requirements
The hardware requirement to use the library is the following:
 - Intel Sapphire Rapids
<!-- - Intel C62XX acceleration card -->

As for the operating systems, the library has been tested and confirmed to work on Ubuntu 20.04.

### Dependencies

Required dependencies include:

```
cmake >=3.15.1
git
yasm
libboost >= 1.14
libudev >= 1.47
pthread
OpenSSL >=1.1.0
gcc >= 9.1
QAT20.L.0.8.0-00071.tar.gz (qatlib and QAT drivers)
ipp-crypto
```

### Instructions

Before attempting to build the library, please check if the platform has the QAT hardware.

```
$ sudo lspci -d 8086:4940
6b:00.0 Co-processor: Intel Corporation Device 4940 (rev 30)
70:00.0 Co-processor: Intel Corporation Device 4940 (rev 30)
75:00.0 Co-processor: Intel Corporation Device 4940 (rev 30)
7a:00.0 Co-processor: Intel Corporation Device 4940 (rev 30)
e8:00.0 Co-processor: Intel Corporation Device 4940 (rev 30)
ed:00.0 Co-processor: Intel Corporation Device 4940 (rev 30)
f2:00.0 Co-processor: Intel Corporation Device 4940 (rev 30)
f7:00.0 Co-processor: Intel Corporation Device 4940 (rev 30)
```

In the example above, the platform is a dual-socket Sapphire Rapids (SPR) and it shows 8 QAT endpoints, 4 on each socket.

#### Installing Dependencies

```
sudo apt install yasm zlib1g 
sudo apt update -y 
sudo apt install -y libsystemd-dev
sudo apt install -y pciutils (tested with version=3.6.4)
sudo apt install -y libudev-dev
sudo apt install -y libreadline-dev
sudo apt install -y libxml2-dev
sudo apt install -y libboost-dev
sudo apt install -y elfutils libelf-dev
sudo apt install -y libnl-3-dev
sudo apt install -y linux-headers-$(uname -r)
sudo apt install -y build-essential
sudo apt install -y libboost-regex-dev
sudo apt install -y pkg-config
```

#### Installing OpenSSL

```
$ git clone https://github.com/openssl/openssl.git
$ cd openssl/
$ git checkout OpenSSL_1_1_1-stable
$ ./Configure --prefix=/opt/openssl
$ make
$ sudo make install
```

#### Installing QAT Software Stack

```
$ cd $HOME
$ mkdir QAT
$ mv QAT20.L.0.8.0-00071.tar.gz QAT/
$ cd QAT
$ tar zxvf QAT20.L.0.8.0-00071.tar.gz
$ ./configure
$ sudo make -j
$ sudo make install
```

Add `$USER` to the `qat` group. Must logout and log back in to take effect. 

```
$ sudo usermod -aG qat $USER
```

> _**Note**_: Please contact the QAT team listed at [https://01.org/intel-quickassist-technology](https://01.org/intel-quickassist-technology) to obtain the latest `QAT20.L.0.8.0-00071.tar.gz` package.

Verify the QAT installation by checking the QAT service status:


```
sudo service qat_service status
```

If all checks out, following the instructions below to build the HE QAT library.

#### Building the Library

Without `BigNumber` support:

```
$ git clone https://github.com/intel-sandbox/libraries.security.cryptography.homomorphic-encryption.glade.project-destiny.git
$ git checkout development
$ export ICP_ROOT=$HOME/QAT
$ cmake -S . -B build -DHE_QAT_MISC=OFF
$ cmake --build build
$ cmake --install build
```

The cmake configuration variable `HE_QAT_MISC=ON` enables `BigNumber` resources and samples, requiring IPP Crypto installation as a dependency. If usage of the utility functions that support `BigNumber` data type is needed, follow the building instructions below to install IPP Crypto and then rebuild the library with the cmake flag `HE_QAT_MISC=ON`:

```
$ git clone https://github.com/intel/ipp-crypto.git
$ cd ipp-crypto
$ CC=gcc CXX=g++ cmake CMakeLists.txt -B_build -DARCH=intel64 -DMERGED_BLD:BOOL=ON -DCMAKE_INSTALL_PREFIX=/opt/ipp-crypto -DOPENSSL_INCLUDE_DIR=/opt/openssl/include -DOPENSSL_LIBRARIES=/opt/openssl/lib -DOPENSSL_ROOT_DIR=/opt/openssl -DCMAKE_ASM_NASM_COMPILER=/opt/nasm-2.15/bin/nasm
$ cmake --build _build -j
$ sudo cmake --install _build
```

#### Configure QAT endpoints

Before trying to run any application or example that uses the HE QAT Lib, the QAT endpoints must be configured. Examples of configurations can be found in the directory `config`. The configuration that we found to serve us with the best performance is located at `config/1inst1dev`.

```
$ sudo cp config/1inst1dev/4xxx_dev*.conf /etc/
$ sudo service qat_service restart
```

#### Configuration Options

In addition to the standard CMake configuration options, Intel HE Acceleration Library for QAT supports several cmake options to configure the build. For convenience, they are listed below:

<!-- | CMake option                  | Values                 |                                                                            | -->
<!-- | ------------------------------| ---------------------- | -------------------------------------------------------------------------- | -->
| HE_QAT_MISC              | ON / OFF (default OFF) | Set to OFF, enable benchmark suite via Google benchmark                   | -->
<!-- | HE_QAT_DEBUG             | ON / OFF (default OFF) | Set to OFF, enable debug log at large runtime penalty                      | -->
<!-- | HE_QAT_SAMPLES                  | ON / OFF (default ON) | Set to ON, enable building of samples.                                  | -->
<!-- | HE_QAT_DOCS                   | ON / OFF (default OFF) | Set to OFF, enable building of documentation                               | -->

#### Running Samples

Test showing creation and teardown of the QAT runtime environment:

```
./build/samples/test_context
``` 

Test showing functional correctness and performance:

```
./build/samples/test_bnModExpPerformOp
``` 

If built with `HE_QAT_MISC=ON`, then the following samples below are also available to try. 

Test showing data conversion between `BigNumber` and `CpaFlatBuffer` formats:

```
./build/samples/test_bnConversion
``` 

Test showing functional correctness and performance using `BigNumber` data types:

```
./build/samples/test_bnModExp
```

## Testing and Benchmarking

TODO

# Contributors

Fillipe D. M. de Souza (Lead)

