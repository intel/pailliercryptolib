# Example using Intel Paillier Cryptosystem Library

The README provides an example program for using the Intel Paillier Cryptosystem Library in an external application.

## Installation

To install the library,
```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/path/to/install/ -DCMAKE_BUILD_TYPE=Release -DIPCL_TEST=OFF -DIPCL_BENCHMARK=OFF
cmake --build build -j
cmake --build build --target install
```

If ```CMAKE_INSTALL_PREFIX``` is not explicitly set, the library will automatically set to ```/opt/intel/ipcl``` by default.

For more details about the build configuration options, please refer to the build instructions provided in the [README.md](../README.md).

## Linking and Running Applications
Before proceeding after the library is installed, it is useful to setup an environment variable to point to the installation location.
```bash
export IPCL_DIR=/path/to/install/
```

### Using g++/clang compiler
In order to directly use `g++` or `clang++` to compile an example code, it can be done by:
```bash
# gcc
g++ test.cpp -o test -L${IPCL_DIR}/lib -I${IPCL_DIR}/include -lipcl -fopenmp -lnuma -lcrypto

# clang
clang++ test.cpp -o test -L${IPCL_DIR}/lib -I${IPCL_DIR}/include -lipcl -fopenmp -lnuma -lcrypto
```

### CMake
A more convenient way to use the library is via the `find_package` functionality in `CMake`.
In your external applications, add the following lines to your `CMakeLists.txt`.

```bash
find_package(IPCL 1.1.4
    HINTS ${IPCL_HINT_DIR}
    REQUIRED)
target_link_libraries(${TARGET} IPCL::ipcl)
```

If the library is installed globally, `IPCL_DIR` or `IPCL_HINT_DIR` flag is not needed. If `IPCL_DIR` is properly set, `IPCL_HINT_DIR` is not needed as well. Otherwise `IPCL_HINT_DIR` should be the directory containing `IPCLCOnfig.cmake`, under `${CMAKE_INSTALL_PREFIX}/lib/cmake/ipcl-1.1.4/`
