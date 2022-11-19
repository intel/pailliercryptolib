# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15.1)

project(IPCL VERSION 1.1.4 LANGUAGES C CXX)

include(CMakePackageConfigHelpers)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(GNUInstallDirs)

if(CMAKE_BUILD_TYPE)
  set(RELEASE_TYPES
      Debug
      Release
      RelWithDebInfo
      MinSizeRel)
  list(FIND RELEASE_TYPES ${CMAKE_BUILD_TYPE} INDEX_FOUND)
  if(${INDEX_FOUND} EQUAL -1)
    message(
      FATAL_ERROR
        "CMAKE_BUILD_TYPE must be one of Debug, Release, RelWithDebInfo, or MinSizeRel"
      )
  endif()
else()
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INSTALL_MESSAGE LAZY)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/opt/intel/ipcl")
endif()

set(CMAKE_C_FLAGS "-O2 -Wno-error=deprecated-declarations")
set(CMAKE_CXX_FLAGS "-O2 -fpermissive -Wno-error=deprecated-declarations")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR};${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/ippcrypto")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_INSTALL_LIBDIR})

#---------------------------------------------------
option(IPCL_TEST "Enable testing" ON)
option(IPCL_BENCHMARK "Enable benchmark" ON)
option(IPCL_ENABLE_OMP "Enable OpenMP testing/benchmarking" ON)
option(IPCL_THREAD_COUNT "The max number of threads used by OpenMP(If the value is OFF/0, it is determined at runtime)" OFF)
option(IPCL_DOCS "Enable document building" OFF)
option(IPCL_SHARED "Build shared library" ON)
option(IPCL_DETECT_IFMA_RUNTIME "Detect AVX512/IFMA instructions during runtime" OFF)
option(IPCL_DEBUG_DISABLE_AVX512IFMA "(Debugging) Disable usage of AVX512IFMA instructions" OFF)
if(IPCL_ENABLE_OMP)
	add_compile_definitions(IPCL_USE_OMP)
    if(IPCL_THREAD_COUNT)
	    add_compile_definitions(IPCL_NUM_THREADS=${IPCL_THREAD_COUNT})
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(IPCL_DEBUG ON)
else()
  set(IPCL_DEBUG OFF)
endif()

set(IPCL_CMAKE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/ipcl")

message(STATUS "CMAKE_BUILD_TYPE:           ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_C_COMPILER:           ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER:         ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_INSTALL_PREFIX:       ${CMAKE_INSTALL_PREFIX}")
message(STATUS "CMAKE_INSTALL_LIBDIR:       ${CMAKE_INSTALL_FULL_LIBDIR}")
message(STATUS "CMAKE_INSTALL_INCLUDEDIR:   ${CMAKE_INSTALL_FULL_INCLUDEDIR}")
message(STATUS "IPCL_TEST:                  ${IPCL_TEST}")
message(STATUS "IPCL_BENCHMARK:             ${IPCL_BENCHMARK}")
message(STATUS "IPCL_ENABLE_OMP:            ${IPCL_ENABLE_OMP}")
if (IPCL_ENABLE_OMP)
  message(STATUS "IPCL_THREAD_COUNT:          ${IPCL_THREAD_COUNT}")
else()
  message(STATUS "IPCL_THREAD_COUNT:          IGNORE")
endif()
message(STATUS "IPCL_DOCS:                  ${IPCL_DOCS}")
message(STATUS "IPCL_SHARED:                ${IPCL_SHARED}")
message(STATUS "IPCL_DETECT_IFMA_RUNTIME:   ${IPCL_DETECT_IFMA_RUNTIME}")

set(IPCL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(IPCL_SRC_DIR ${IPCL_ROOT_DIR}/ipcl)
set(IPCL_INC_DIR ${IPCL_SRC_DIR}/include)

set(IPCL_FORWARD_CMAKE_ARGS
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
    -DCMAKE_CXX_STANDARD_REQUIRED=${CMAKE_CXX_STANDARD_REQUIRED}
    -DCMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
    -DCMAKE_EXPORT_COMPILE_COMMANDS=${CMAKE_EXPORT_COMPILE_COMMANDS}
    -DCMAKE_POSITION_INDEPENDENT_CODE=${CMAKE_POSITION_INDEPENDENT_CODE}
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

if(IPCL_DETECT_IFMA_RUNTIME)
  add_compile_definitions(IPCL_RUNTIME_MOD_EXP)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/cpufeatures")
else()
  # check whether cpu support avx512 flag
  if(IPCL_DEBUG_DISABLE_AVX512IFMA)
    message(STATUS "Support AVX512IFMA: False")
  else()
    set(CPU_AVX512_FLAG "avx512ifma")
    execute_process(COMMAND lscpu COMMAND grep ${CPU_AVX512_FLAG}  OUTPUT_VARIABLE CPU_ENABLE_AVX512)
    if("${CPU_ENABLE_AVX512}" STREQUAL "")
      message(STATUS "Support AVX512IFMA: False")
    else()
      message(STATUS "Support AVX512IFMA: True")
      add_compile_definitions(IPCL_CRYPTO_MB_MOD_EXP)
    endif()
  endif()
endif()

# check whether cpu support rdseed or rdrand instruction
set(CPU_RDSEED_FLAG "rdseed")
execute_process(COMMAND lscpu COMMAND grep ${CPU_RDSEED_FLAG}  OUTPUT_VARIABLE CPU_ENABLE_RDSEED)
if("${CPU_ENABLE_RDSEED}" STREQUAL "")
  set(CPU_RDRAND_FLAG "rdrand")
  execute_process(COMMAND lscpu COMMAND grep ${CPU_RDRAND_FLAG}  OUTPUT_VARIABLE CPU_ENABLE_RDRAND)
  if("${CPU_ENABLE_RDRAND}" STREQUAL "")
    message(WARNING "CPU doesn't support RDSEED and RDRAND instruction, using random generator will cause errors.")
  else ()
    message(STATUS "Support RDRAND instruction: True")
    add_compile_definitions(IPCL_RNG_INSTR_RDRAND)
  endif()
else()
  message(STATUS "Support RDSEED instruction: True")
  add_compile_definitions(IPCL_RNG_INSTR_RDSEED)
endif()

# find package for OpenSSL and Threads
# set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# External dependencies
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ipcl)
include(ipcl-util)

include(cmake/ippcrypto.cmake)
if(IPCL_DETECT_IFMA_RUNTIME)
  include(cmake/cpufeatures.cmake)
endif()

if(IPCL_TEST)
  include(cmake/gtest.cmake)
endif()
if(IPCL_BENCHMARK)
  include(cmake/gbenchmark.cmake)
endif()



add_subdirectory(ipcl)

# unit-test and benchmarks
if(IPCL_TEST)
  add_subdirectory(test)
  add_custom_target(unittest COMMAND $<TARGET_FILE:unittest_ipcl> DEPENDS unittest_ipcl)
endif()
unset(IPCL_TEST CACHE)

if(IPCL_BENCHMARK)
  add_subdirectory(benchmark)
  add_custom_target(benchmark COMMAND $<TARGET_FILE:bench_ipcl> DEPENDS bench_ipcl)
endif()

if(IPCL_DOCS)
  add_subdirectory(docs)
endif()

unset(IPCL_BENCHMARK CACHE)
