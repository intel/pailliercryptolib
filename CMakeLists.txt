# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15.1)

project(IPCL VERSION 1.0.0 LANGUAGES C CXX)

include(CMakePackageConfigHelpers)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

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
set(CMAKE_INSTALL_RPATH "\$ORIGIN")

#set(CMAKE_C_FLAGS "-O2 -pthread")
#set(CMAKE_CXX_FLAGS "-O2 -fpermissive -pthread")
set(CMAKE_C_FLAGS "-pthread")
set(CMAKE_CXX_FLAGS "-fpermissive -pthread")

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
endif()

include(GNUInstallDirs)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_INSTALL_LIBDIR})

#---------------------------------------------------
option(IPCL_TEST "Enable testing" ON)
option(IPCL_BENCHMARK "Enable benchmark" ON)
option(IPCL_ENABLE_QAT "Enable QAT" OFF)
option(IPCL_USE_QAT_LITE "Enable uses QAT for base and exponent length different than modulus" OFF)
option(IPCL_ENABLE_OMP "Enable OpenMP testing/benchmarking" ON)
option(IPCL_DOCS "Enable document building" OFF)
option(IPCL_SHARED "Build shared library" ON)

if(IPCL_ENABLE_QAT)
	add_compile_definitions(IPCL_USE_QAT)
  message(STATUS "QAT enabled - IPCL_ENABLE_OMP set to OFF")
  set(IPCL_ENABLE_OMP OFF)
  if(IPCL_USE_QAT_LITE)
    add_compile_definitions(IPCL_USE_QAT_LITE)
    message(STATUS "QAT Lite enabled - IPCL_USE_QAT_LITE set to ON")
  else()
    message(STATUS "QAT Lite disabled - IPCL_USE_QAT_LITE set to OFF")
  endif()
endif()

if(IPCL_ENABLE_OMP)
	add_compile_definitions(IPCL_USE_OMP)
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
message(STATUS "IPCL_TEST:                  ${IPCL_TEST}")
message(STATUS "IPCL_BENCHMARK:             ${IPCL_BENCHMARK}")
message(STATUS "IPCL_ENABLE_OMP:            ${IPCL_ENABLE_OMP}")
message(STATUS "IPCL_ENABLE_QAT:            ${IPCL_ENABLE_QAT}")
message(STATUS "IPCL_DOCS:                  ${IPCL_DOCS}")
message(STATUS "IPCL_SHARED:                ${IPCL_SHARED}")

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

# find package for OpenSSL and Threads
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# External dependencies
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ipcl)
include(ipcl-util)

include(cmake/ippcrypto.cmake)

if(IPCL_ENABLE_QAT)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/heqat/heqat.cmake)
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
