# How to Build Intel® Paillier Cryptosystem Library

## Requirements
### Tools and dependencies
- [CMake](https://cmake.org/download) >= 3.15.1
- Python >= 3.6.5
- [Netwide Assembler (nasm)](https://www.nasm.us/pub/nasm/releasebuilds/) >= 2.14.02
- OpenSSL >= 1.1.0
- g++ >= 8.3 or clang >=9.0
- GNU binutils >= 2.32
- numa >= 2.0.12


## Building Intel Paillier Cryptosystem Library

The library was validated on:
- Ubuntu 18.04 and higher
- Red Hat Enterprise Linux 7
- CentOS 8
- Pop!_OS 20.04 and higher

To build the Intel Paillier Cryptosystem Library, complete the following steps:
1. Clone the source code from Github as follows:
```bash
git clone https://github.com/intel/pailliercryptolib.git
```
2. Configure the library with CMake:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```
For the complete set of supported CMake options, refer to the [CMake Build Options](#cmake-build-options) section.

3. Start the build:
```bash
cmake --build build -j$(nproc)
```

### Installing Intel Paillier Cryptosystem Library
For installation, due to an external dependency, ```IPP-Crypto```, it is only possible to properly install the library with the static build option.

To build and install the Intel Paillier Cryptosystem Library, complete the following steps:
1. Configure the library with the following CMake flags:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DIPCL_TEST=OFF -DIPCL_BENCHMARK=OFF -DIPCL_SHARED=OFF -DCMAKE_INSTALL_PREFIX=/path/to/install/lib/
```
If ```CMAKE_INSTALL_PREFIX``` is not specified, by default, it will be set to ```/usr/local/lib/```.

2.


## CMake Build Options
It is possible to pass additional options to enable more features. The following table contains the current CMake options, default values are in bold.

| CMake options           | Values    | Default | Comments                            |
|-------------------------|-----------|---------|-------------------------------------|
|`IPCL_TEST`              | ON/OFF    | ON      | unit-test                           |
|`IPCL_BENCHMARK`         | ON/OFF    | ON      | benchmark                           |
|`IPCL_ENABLE_OMP`        | ON/OFF    | ON      | enables OpenMP functionalities      |
|`IPCL_THREAD_COUNT`      | Integer   | OFF     | The max number of threads           |
|`IPCL_DOCS`              | ON/OFF    | OFF     | build doxygen documentation         |
|`IPCL_SHARED`            | ON/OFF    | ON      | build shared library                |
