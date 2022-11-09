# Intel Homomorphic Encryption (HE) Acceleration Library for Quick Assist Technology (QAT)
Intel Homomorphic Encryption Acceleration Library for QAT (HE QAT Lib) is an open-source library which provides accelerated performance for homomorphic encryption (HE) math functions involving multi-precision numbers and modular arithmetic. This library is written in C99.

## Contents
- [Intel Homomorphic Encryption (HE) Acceleration Library for Quick Assist Technology (QAT)](#intel-homomorphic-encryption-he-acceleration-library-for-quick-assist-technology-qat)
  - [Contents](#contents)
  - [Introduction](#introduction)
  - [Building the HE QAT Library](#building-the-he-qat-library)
    - [Requirements](#requirements)
    - [Dependencies](#dependencies)
    - [Instructions](#instructions)
      - [Installing Dependencies](#installing-dependencies)
      - [Installing OpenSSL](#installing-openssl)
      - [Installing QAT Software Stack](#installing-qat-software-stack)
      - [Setup Environment](#setup-environment)
      - [Building the Library](#building-the-library)
      - [Configuring QAT endpoints](#configuring-qat-endpoints)
      - [Configuration Options](#configuration-options)
      - [Running Samples](#running-samples)
      - [Running All Samples](#running-all-samples)
  - [Troubleshooting](#troubleshooting)
  - [Testing and Benchmarking](#testing-and-benchmarking)
- [Contributors](#contributors)
<!-- - [Standardization](#standardization) -->
- [Contributors](#contributors)

## Introduction

This library currently only offers acceleration of modular exponentiation of multi-precision numbers, i.e. large numbers whose precision range from 1024 to 8192 bits. Current stage of implementation supports modular exponentiation of big numbers encoded with OpenSSL `BIGNUM` data type, `ippcrypto`'s `BigNumber` class and octet strings encoded with `unsigned char`. More details about the modes of operation and characteristics of the execution flow are described below:

 - Synchronous: API calls will submit requests that will be executed in the order they are first issued by the host caller, i.e. a series of modular exponentiation operation requests will be offloaded for processing by the accelerator in the order they are issued.

 - Asynchronous: API calls will submit requests that will NOT necessarily be executed in the order they are first issued by the host caller, i.e. a sequence of multiple requests for the modular exponentiation operation could be scheduled out of order and executed concurrently by the accelerator; thus, completed out of order.

 - Blocking: API calls will be blocked until work request processing completion. Internally, the next buffered work request waits for completion of the processing of the most recently offloaded request to the accelerator.

  - Non-Blocking: API calls will be non-blocking, it does not wait for completion of the work request to return from call. After multiple non-blocking calls to the API, a blocking function to wait for the requests to complete processing must be called. Internally, non-blocking request submissions are scheduled to the accelerator asynchronously. When there are multiple requests from concurrent API callers, the requests are not guaranteed to be processed in order of arrival.

 - Batch Support: The internal buffers are set accommodate up to 1024 requests at a time so that the maximum number of non-blocking API calls is 1024 for each concurrent thread caller. Therefore, only up to 1024 requests can be exercised asynchronously from the application side, be it from a single `for loop` or static code block. Finally, in order to collect the requests, a call to the `getBnModExpRequest()` function must be performed to wait for completion of all submitted asynchronous requests. On multithreaded mode, the blocking function to be called at the end of the code block shall be `release_bnModExp_buffer()`.

 - Multithreading Support: This feature permits the API to be called by concurrently threads running on the host. Effective multithreading support relies on a separate buffer that admits outstanding work requests. This buffer is acquired before an API call to submit work requests to the accelerator. This is accomplished by first calling `acquire_bnModExp_buffer()` to reserve an internal buffer to store outstanding requests from the host API caller.

 - Multiple Instances: The library accesses all logical instances from all visible and configured QAT endpoints at the creation of the QAT runtime context. Therefore, if 8 QAT endpoints are available, it will attempt to use them all, including all the total number of logical instances configured per process.

>> _**Note**_: Current implementation does not verify if the instance/endpoint has the capabilities needed by the library. For example, the library needs access to the _asym_ capabilities like `CyLnModExp`, therefore if the configuration file of an endpoint happens to be configured to not offer it, the application will exit with an error at some point during execution.

## Building the HE QAT Library

### Requirements
The hardware requirement to use the library is the following:
 - Intel 4xxx co-processor
<!-- - Intel C62XX acceleration card -->

As for the operating systems, the library has been tested and confirmed to work on Ubuntu 20.04 and CentOS 7.9.

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
nasm >= 2.15
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

In the example above, the platform is a dual-socket server with Sapphire Rapids (SPR) CPU and it shows 8 QAT endpoints, 4 on each socket.

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

 - Ubuntu
```
sudo service qat_service status
```
 - CentOS
```
sudo systemctl status qat_service.service
```

If all checks out, following the instructions below to build the HE QAT library.

#### Setup Environment

This step is required. Note that if the step [Installing QAT Software Stack](#installing-qat-software-stack) has just been performed, then the exact path of the installation is known, i.e.

```
export ICP_ROOT=$HOME/QAT
```

Alternatively, if the system has a pre-built QAT software stack installed, the script `auto_find_qat_install.sh` can used to help automatically find the path where it was installed (see command below). The script `auto_find_qat_install.sh` assumes that the QAT package is installed in a single location, such that if multiple installations are available at different locations, the script may produce undetermined behavior.

 - Explicit way:
```
export ICP_ROOT=$(./auto_find_qat_install.sh)
```
 - Implicit way:
```
source setup_env.sh
```

#### Building the Library

Follow the steps in the sections [Installing QAT Software Stack](#installing-qat-software-stack) and [Setup Environment](#setup-environment) before attempting to build the library.

- How to build without `BigNumber` support

```
$ git clone https://github.com/intel-sandbox/libraries.security.cryptography.homomorphic-encryption.glade.project-destiny.git
$ git checkout development
$ cmake -S . -B build -DHE_QAT_MISC=OFF
$ cmake --build build
$ cmake --install build
```

- How to build with `BigNumber` support

The `cmake` configuration variable `HE_QAT_MISC=ON` enables `BigNumber` resources and samples, requiring IPP Crypto installation as a dependency. If usage of the utility functions that support `BigNumber` data type is needed, follow the building instructions below to install IPP Crypto and then rebuild the library with the cmake flag `HE_QAT_MISC=ON`:

- Installing `nasm-2.15`

```
$ wget -c https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.xz
$ tar -xf nasm-2.15.05.tar.xz
$ cd nasm-2.15.05/
$ ./configure --prefix=/opt/nasm-2.15
$ make -j
$ sudo make install
```

- Installing `ippcrypto`

```
$ cd ~
$ git clone https://github.com/intel/ipp-crypto.git
$ cd ipp-crypto
$ CC=gcc CXX=g++ cmake CMakeLists.txt -B_build -DARCH=intel64 -DMERGED_BLD:BOOL=ON -DCMAKE_INSTALL_PREFIX=/opt/ipp-crypto -DOPENSSL_INCLUDE_DIR=/opt/openssl/include -DOPENSSL_LIBRARIES=/opt/openssl/lib -DOPENSSL_ROOT_DIR=/opt/openssl -DCMAKE_ASM_NASM_COMPILER=/opt/nasm-2.15/bin/nasm
$ cmake --build _build -j
$ sudo cmake --install _build
```

#### Configuring QAT endpoints

Before trying to run any application or example that uses the HE QAT Lib, the QAT endpoints must be configured.
The default configuration provided in this release is the optimal configuration to provide computing acceleration support for [IPCL](https://github.com/intel/pailliercryptolib).
The boilerplate configurations can be found in the `config` directory.

```
./scripts/setup_devices.sh
```

The script above will configure the QAT devices to perform asymmetric functions only.

#### Configuration Options

In addition to the standard CMake configuration options, Intel HE Acceleration Library for QAT supports several cmake options to configure the build. For convenience, they are listed below:

| CMake option                  | Values                 | Description                                             |
| ------------------------------| ---------------------- | ------------------------------------------------------- |
| HE_QAT_MISC                   | ON / OFF (default OFF) | Enable/Disable BigNumber conversion functions.          |
| HE_QAT_DEBUG                  | ON / OFF (default OFF) | Enable/Disable debug log at large runtime penalty.      |
| HE_QAT_SAMPLES                | ON / OFF (default ON)  | Enable/Disable building of samples.                     |
| HE_QAT_DOCS                   | ON / OFF (default ON)  | Enable/Disable building of documentation.               |
| HE_QAT_SYNC                   | ON / OFF (default OFF) | Enable/Disable synchronous mode execution.              |
| HE_QAT_MT                     | ON / OFF (default ON)  | Enable/Disable interfaces for multithreaded programs.   |
| HE_QAT_PERF                   | ON / OFF (default OFF) | Enable/Disable display of measured request performance. |
| HE_QAT_TEST                   | ON / OFF (default OFF) | Enable/Disable testing.                                 |
| HE_QAT_OMP                    | ON / OFF (default ON)  | Enable/Disable tests using OpenMP.                      |
| HE_QAT_SHARED                 | ON / OFF (default ON)  | Enable/Disable building shared library.                 |

#### Running Samples

Test showing creation and teardown of the QAT runtime environment:

```
./build/samples/sample_context
```

Test showing functional correctness and performance using BIGNUM data as input:

```
./build/samples/sample_BIGNUMModExp
```

If built with `HE_QAT_MISC=ON`, then the following samples below are also available to try.

Test showing data conversion between `BigNumber` and `CpaFlatBuffer` formats:

```
./build/samples/sample_bnConversion
```

Test showing functional correctness and performance using `BigNumber` data types:

```
./build/samples/sample_bnModExp
```

Test showing functional correctness and performance of multithreading support:

```
./build/samples/sample_bnModExp_MT
```
#### Running All Samples

```
HEQATLIB_ROOT_DIR=$PWD ./scripts/run.sh
```

## Troubleshooting

- **Issue #1**

```
xuser@ubuntu-guest:~/heqat$ cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHE_QAT_MISC=ON
-- CMAKE_INSTALL_PREFIX: /usr/local
-- CMAKE_PREFIX_PATH /home/xuser/ipp-crypto/_build/
-- Missed required Intel IPP Cryptography component: ippcp
--   library not found:
   /opt/ipp-crypto/lib/intel64/libippcp.a
CMake Error at CMakeLists.txt:93 (find_package):
  Found package configuration file:

    /opt/ipp-crypto/lib/cmake/ippcp/ippcp-config.cmake

  but it set IPPCP_FOUND to FALSE so package "IPPCP" is considered to be NOT
  FOUND.
```

To resolve the error below simply create the symbolic link `/opt/ipp-crypto/lib/intel64/libippcp.a` from the appropriate static ippcp library that was compiled. For example:

```
xuser@ubuntu-guest:/opt/ipp-crypto/lib/intel64$ ls -lha
total 7.3M
drwxr-xr-x 2 root root 4.0K Jun  3 16:29 .
drwxr-xr-x 5 root root 4.0K Jun  3 16:29 ..
-rw-r--r-- 1 root root 1.6M Jun  3 16:28 libcrypto_mb.a
lrwxrwxrwx 1 root root   18 Jun  3 16:29 libcrypto_mb.so -> libcrypto_mb.so.11
lrwxrwxrwx 1 root root   20 Jun  3 16:29 libcrypto_mb.so.11 -> libcrypto_mb.so.11.5
-rw-r--r-- 1 root root 1.3M Jun  3 16:28 libcrypto_mb.so.11.5
lrwxrwxrwx 1 root root   16 Jun  3 16:29 libippcpmx.so -> libippcpmx.so.11
lrwxrwxrwx 1 root root   18 Jun  3 16:29 libippcpmx.so.11 -> libippcpmx.so.11.5
-rw-r--r-- 1 root root 1.7M Jun  3 16:28 libippcpmx.so.11.5
-rw-r--r-- 1 root root 2.9M Jun  3 16:28 libippcp_s_mx.a
xuser@ubuntu-guest:/opt/ipp-crypto/lib/intel64$ sudo ln -s libippcp_s_mx.a libippcp.a
```

## Testing and Benchmarking

TODO

# Contributors

Main contributors to this project, sorted by alphabetical order of last name are:
  - [Fillipe Dias M. de Souza](https://www.linkedin.com/in/fillipe-d-m-de-souza-a8281820) (lead)
  - [Xiaoran Fang](https://github.com/fangxiaoran)
  - [Jingyi Jin](https://www.linkedin.com/in/jingyi-jin-655735)
  - [Sejun Kim](https://www.linkedin.com/in/sejun-kim-2b1b4866)
  - [Pengfei Zhao](https://github.com/justalittlenoob)
