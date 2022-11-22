# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15.1)

project(IPCL VERSION 2.0.0 LANGUAGES C CXX)

# includes
include(CMakePackageConfigHelpers)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(GNUInstallDirs)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ipcl)
include(ipcl-util)

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

if(CMAKE_PREFIX_PATH)
  message(STATUS "---- CMAKE_PREFIX_PATH is defined as ${CMAKE_PREFIX_PATH}")
else()
  set(CMAKE_PREFIX_PATH $ENV{HOME}/intel /opt/intel)
endif()


set(CMAKE_C_FLAGS "-O2 -Wno-error=deprecated-declarations -Wno-error=deprecated-copy")
set(CMAKE_CXX_FLAGS "-O2 -fpermissive -Wno-error=deprecated-declarations -Wno-error=deprecated-copy")
set(CMAKE_INSTALL_RPATH "$ORIGIN;$ORIGIN/${CMAKE_INSTALL_LIBDIR};$ORIGIN/ippcrypto")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_INSTALL_LIBDIR})

# Compiler version check - icx/icpx-2021.3.0 is supported
if(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 2021.3.0)
    message(FATAL_ERROR
    " ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION} is not supported."
    " Please refer to Intel IPP-Crypto (https://github.com/intel/ipp-crypto"
    " for more information.")
  endif()
endif()

#---------------------------------------------------
option(IPCL_TEST "Enable testing" ON)
option(IPCL_BENCHMARK "Enable benchmark" ON)
option(IPCL_ENABLE_QAT "Enable QAT" OFF)
option(IPCL_USE_QAT_LITE "Enable uses QAT for base and exponent length different than modulus" OFF)
option(IPCL_ENABLE_OMP "Enable OpenMP testing/benchmarking" ON)
option(IPCL_THREAD_COUNT "The max number of threads used by OpenMP(If the value is OFF/0, it is determined at runtime)" OFF)
option(IPCL_DOCS "Enable document building" OFF)
option(IPCL_SHARED "Build shared library" ON)
option(IPCL_DETECT_CPU_RUNTIME "Detect CPU supported instructions during runtime" OFF)
option(IPCL_INTERNAL_PYTHON_BUILD "Additional steps for IPCL_Python build" OFF)

# Used only for ipcl_python IPCL_INTERNAL_PYTHON_BUILD - additional check if invalid parameters
if(IPCL_INTERNAL_PYTHON_BUILD)
  if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(IPCL_INTERNAL_PYTHON_BUILD OFF)
  elseif(NOT IS_ABSOLUTE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set(IPCL_INTERNAL_PYTHON_BUILD OFF)
  endif()
endif()

if(IPCL_ENABLE_QAT)
  ipcl_detect_qat()
  if(IPCL_FOUND_QAT)
    add_compile_definitions(IPCL_USE_QAT)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};$ORIGIN/../heqat")
    if(IPCL_USE_QAT_LITE)
      add_compile_definitions(IPCL_USE_QAT_LITE)
      message(STATUS "QAT Lite enabled - IPCL_USE_QAT_LITE set to ON")
    else()
      message(STATUS "QAT Lite disabled - IPCL_USE_QAT_LITE set to OFF")
    endif()
  else()
    set(IPCL_ENABLE_QAT OFF)
  endif()
endif()

if(IPCL_THREAD_COUNT LESS_EQUAL 1)
  set(IPCL_ENABLE_OMP OFF)
endif()

if(IPCL_ENABLE_OMP)
	add_compile_definitions(IPCL_USE_OMP)
  ipcl_get_core_thread_count(num_cores num_threads num_nodes)
  if(IPCL_THREAD_COUNT)
    # if thread_count is invalid, set to maximum threads
    if(IPCL_THREAD_COUNT GREATER num_threads)
      set(IPCL_THREAD_COUNT ${num_threads})
    endif()
    add_compile_definitions(IPCL_NUM_THREADS=${IPCL_THREAD_COUNT})
  endif()
endif()

if(IPCL_DETECT_CPU_RUNTIME)
  # add_compile_definitions(IPCL_RUNTIME_MOD_EXP)
  add_compile_definitions(IPCL_RUNTIME_DETECT_CPU_FEATURES)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};$ORIGIN/cpufeatures")
else()
  # set cpu node count parsed from lscpu precompile
  add_compile_definitions(IPCL_NUM_NODES=${num_nodes})

  # check whether cpu support avx512ifma instructions
  ipcl_detect_lscpu_flag("avx512ifma")
  if(IPCL_FOUND_avx512ifma)
    add_compile_definitions(IPCL_CRYPTO_MB_MOD_EXP)
  endif()

  # check whether cpu support rdseed/rdrand instructions
  ipcl_detect_lscpu_flag("rdseed")
  if(IPCL_FOUND_rdseed)
    add_compile_definitions(IPCL_RNG_INSTR_RDSEED)
  else()
    ipcl_detect_lscpu_flag("rdrand")
    if(IPCL_FOUND_rdrand)
      add_compile_definitions(IPCL_RNG_INSTR_RDRAND)
    else()
      message(WARNING
          "CPU doesn't support RDSEED and RDRAND instruction, using IPP-Crypto"
          " S/W pseudo random number generator"
      )
    endif()
  endif()
endif()

message(STATUS "CMAKE_BUILD_TYPE:           ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_C_COMPILER:           ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER:         ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_INSTALL_PREFIX:       ${CMAKE_INSTALL_PREFIX}")
message(STATUS "IPCL_TEST:                  ${IPCL_TEST}")
message(STATUS "IPCL_BENCHMARK:             ${IPCL_BENCHMARK}")
message(STATUS "IPCL_ENABLE_OMP:            ${IPCL_ENABLE_OMP}")
message(STATUS "IPCL_ENABLE_QAT:            ${IPCL_ENABLE_QAT}")
if (IPCL_ENABLE_OMP)
  message(STATUS "IPCL_THREAD_COUNT:          ${IPCL_THREAD_COUNT}")
else()
  message(STATUS "IPCL_THREAD_COUNT:          IGNORE")
endif()
message(STATUS "IPCL_DOCS:                  ${IPCL_DOCS}")
message(STATUS "IPCL_SHARED:                ${IPCL_SHARED}")
message(STATUS "IPCL_DETECT_CPU_RUNTIME:    ${IPCL_DETECT_CPU_RUNTIME}")
if(IPCL_INTERNAL_PYTHON_BUILD)
  message(STATUS "IPCL_INTERNAL_PYTHON_BUILD: ${IPCL_INTERNAL_PYTHON_BUILD}")
endif()

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

# global IPCL folders
set(IPCL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(IPCL_SRC_DIR ${IPCL_ROOT_DIR}/ipcl)
set(IPCL_INC_DIR ${IPCL_SRC_DIR}/include)
set(IPCL_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/ipcl)
set(IPCL_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/ipcl)
set(IPCL_CMAKE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/ipcl")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(IPCL_DEBUG ON)
else()
  set(IPCL_DEBUG OFF)
endif()

# find package: Threads config
set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads REQUIRED)

# find package: OpenSSL config
find_package(OpenSSL REQUIRED)

# External dependencies
include(cmake/ippcrypto.cmake)
include(cmake/cereal.cmake)

if(IPCL_DETECT_CPU_RUNTIME)
  include(cmake/cpufeatures.cmake)
endif()

if(IPCL_ENABLE_QAT)
  # preset values for including HE_QAT
  set(HE_QAT_MISC OFF)
  set(HE_QAT_DOCS ${IPCL_DOCS})
  set(HE_QAT_SHARED ${IPCL_SHARED})
  set(HE_QAT_TEST OFF)
  add_subdirectory(module/heqat)
endif()

if(IPCL_TEST)
  include(cmake/gtest.cmake)
endif()
if(IPCL_BENCHMARK)
  include(cmake/gbenchmark.cmake)
endif()

# IPCL main directory
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

# doxygen generation
if(IPCL_DOCS)
  add_subdirectory(docs)
endif()

unset(IPCL_BENCHMARK CACHE)
