# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13)

# The hekat or heqat (transcribed HqA.t) was an ancient Egyptian volume unit
# used to measure grain, bread, and beer.
project(HE_QAT VERSION 1.3.2 LANGUAGES C CXX)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(HE_QAT_STANDALONE ON)
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR) # built under IPCL
  set(HE_QAT_STANDALONE OFF)
endif()

if(HE_QAT_STANDALONE)
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

  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(CMAKE_INSTALL_MESSAGE LAZY)

  set(CMAKE_C_FLAGS "-O2 -Wunused-variable -Wunused-function")
  set(CMAKE_CXX_FLAGS "-O2 -Wunused-variable -Wunused-function -fpermissive")
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/install)
  endif()
endif()

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/heqat")

message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_INSTALL_LIBDIR})

# -------------------------------------------------------------------
if(HE_QAT_STANDALONE) # standalone
  option(HE_QAT_MISC "Enable miscellaneous features" ON)
  option(HE_QAT_SYNC "Enable synchronous mode execution" OFF)
  option(HE_QAT_MT "Enable interfaces for multithreaded programs" ON)
  option(HE_QAT_PERF "Show request performance" OFF)
  option(HE_QAT_TEST "Enable testing" ON)
  option(HE_QAT_OMP "Enable tests using OpenMP" ON)
  option(HE_QAT_DOCS "Enable document building" ON)
  option(HE_QAT_SHARED "Build shared library" ON)

  set(HE_QAT_FORWARD_CMAKE_ARGS
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_C_STANDARD=${CMAKE_C_STANDARD}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
    -DCMAKE_CXX_STANDARD_REQUIRED=${CMAKE_CXX_STANDARD_REQUIRED}
    -DCMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
    -DCMAKE_EXPORT_COMPILE_COMMANDS=${CMAKE_EXPORT_COMPILE_COMMANDS}
    -DCMAKE_POSITION_INDEPENDENT_CODE=${CMAKE_POSITION_INDEPENDENT_CODE}
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  )
else()
  option(HE_QAT_SYNC "Enable synchronous mode execution" OFF)
  option(HE_QAT_MT "Enable interfaces for multithreaded programs" ON)
  option(HE_QAT_PERF "Show request performance" OFF)
  option(HE_QAT_OMP "Enable tests using OpenMP" ON)
  set(HE_QAT_FORWARD_CMAKE_ARGS ${IPCL_FORWARD_CMAKE_ARGS})
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(HE_QAT_DEBUG ON)
  add_definitions(-DHE_QAT_DEBUG)
else()
  set(HE_QAT_DEBUG OFF)
endif()

if(HE_QAT_MISC)
  add_definitions(-DHE_QAT_MISC)
endif()

# Why?
set(HE_QAT_CMAKE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/he_qat")
set(HE_QAT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(HE_QAT_SRC_DIR ${HE_QAT_ROOT_DIR}/heqat)
set(HE_QAT_INC_DIR ${HE_QAT_ROOT_DIR}/heqat/include)
if(NOT HE_QAT_STANDALONE)
  set(HE_QAT_INC_DIR ${HE_QAT_INC_DIR} PARENT_SCOPE)
endif()
set(HE_QAT_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
set(HE_QAT_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/heqat)

if(HE_QAT_OMP)
  find_package(OpenMP REQUIRED)
  if(NOT TARGET OpenMP::OpenMP_CXX)
    message(FATAL_ERROR "Missing OpenMP::OpenMP_CXX.")
  endif()
  if(NOT TARGET OpenMP::OpenMP_C)
    message(FATAL_ERROR "Missing OpenMP::OpenMP_C.")
  endif()
endif()

if(HE_QAT_MT)
  add_definitions(-DHE_QAT_MT)
  message(STATUS "Compile with multithreaded interfaces.")
endif()

if(HE_QAT_MISC)
  if(HE_QAT_STANDALONE)
    # IPP Crypto installation
    if(IPPCP_PREFIX_PATH)
      list(APPEND CMAKE_PREFIX_PATH "${IPPCP_PREFIX_PATH}")
      set(IPPCP_DIR "${IPPCP_PREFIX_PATH}/../../../")
      message(STATUS "IPPCP_DIR=${IPPCP_DIR}")
    else()
      # Default to this
      set(IPPCP_DIR "/opt/ipp-crypto")
      set(IPPCP_PREFIX_PATH "${IPPCP_DIR}/lib/cmake")
      list(APPEND CMAKE_PREFIX_PATH "${IPPCP_PREFIX_PATH}")
      message(STATUS "Else IPPCP_DIR=${IPPCP_DIR}")
    endif()
    find_package(IPPCP REQUIRED)
    message(STATUS "IPPCP_LIBRARIES ${IPPCP_LIBRARIES}")
    set(IPPCP_INC_DIR ${IPPCP_DIR}/../../../include)
    set(IPPCP_LIB_DIR ${IPPCP_DIR}/lib/intel64)
  else()
    set(IPPCP_INC_DIR ${IPPCRYPTO_INC_DIR}/ippcrypto)
    set(IPPCP_LIB_DIR ${IPPCRYPTO_LIB_DIR})
  endif()
endif()

if(HE_QAT_SYNC)
  add_definitions(-DHE_QAT_SYNC_MODE)
endif()

if(HE_QAT_PERF)
  add_definitions(-DHE_QAT_PERF)
endif()

# OpenSSL installation
find_package(OpenSSL REQUIRED)

# External dependencies
find_package(Threads REQUIRED)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/he_qat)
include(heqat-util)

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/install)
endif()

# Include QAT lib API support
include(cmake/qatconfig.cmake)

# HE_QAT Library
add_subdirectory(heqat)

#Validation test examples
if(HE_QAT_TEST)
  add_subdirectory(test)
endif()

if(HE_QAT_DOCS)
  # sudo apt-get install doxygen
  find_package(Doxygen)
  option(BUILD_DOCS "Create and install the HTML based API docs (requires Doxygen)" ${DOXYGEN_FOUND})
  if(BUILD_DOCS)
    if(NOT DOXYGEN_FOUND)
      message(FATAL_ERROR "Doxygen was not found (Required)")
    else()
      add_subdirectory(doc)
    endif()
  endif()
endif()
