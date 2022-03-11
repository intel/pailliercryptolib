cmake_minimum_required(VERSION 3.13)

# The hekat or heqat (transcribed HqA.t) was an ancient Egyptian volume unit 
# used to measure grain, bread, and beer.
project(HE_QAT VERSION 0.1.0 LANGUAGES C CXX)

include(CheckCCompilerFlag) 
include(CheckCXXCompilerFlag) 
include(CMakePackageConfigHelpers)

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

set(CMAKE_C_FLAGS "-O2")
set(CMAKE_CXX_FLAGS "-O2 -fpermissive")

# What does it do?
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# When to use it?
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Why?
set(CMAKE_INSTALL_MESSAGE LAZY)
# Why?
set(CMAKE_INSTALL_RPATH "\$ORIGIN")

#if(NOT CMAKE_INSTALL_PREFIX)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/install)
	#endif()
message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

# Which features or functions does it bring?
include(GNUInstallDirs)

# What is CMAKE_ARCHIVE_OUTPUT_DIRECTORY?
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_INSTALL_LIBDIR})

# -------------------------------------------------------------------

option(HE_QAT_MISC "Enable miscellaneous features" ON)
option(HE_QAT_TEST "Enable testing" OFF)
option(HE_QAT_EXAMPLES "Enable examples" ON)
option(HE_QAT_BENCHMARK "Enable benchmark" OFF)
option(HE_QAT_DOCS "Enable document building" OFF)
option(HE_QAT_SHARED "Build shared library" ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(HE_QAT_DEBUG ON)
else()
  set(HE_QAT_DEBUG OFF)
endif()

# Why?
set(HE_QAT_CMAKE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/he_qat")

set(HE_QAT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}) 
set(HE_QAT_SRC_DIR ${HE_QAT_ROOT_DIR}/he_qat) 
set(HE_QAT_INC_DIR ${HE_QAT_ROOT_DIR}/he_qat/include)

#OpenSSL installation
find_package(OpenSSL REQUIRED)

# External dependencies
find_package(Threads REQUIRED)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
#set(OPENSSL_INC_DIR /opt/openssl/include) 
#set(OPENSSL_LIB_DIR /opt/openssl/lib)

set(COMMON_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/include)
if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/install)
endif()

# Include QAT lib API support
include(icp/CMakeLists.txt)

#Common utility functions
add_subdirectory(common)

if(HE_QAT_MISC)
  # IPP Crypto installation
  if(NOT IPPCP_CMAKE_PREFIX_PATH)
    set(IPPCP_DIR "/opt/ipp-crypto")
    set(IPPCP_CMAKE_PREFIX_PATH "${IPPCP_DIR}/lib/cmake")
    list(APPEND CMAKE_PREFIX_PATH "${IPPCP_CMAKE_PREFIX_PATH}")
  endif()

  find_package(IPPCP REQUIRED)
 
  message(STATUS "IPPCP_LIBRARIES ${IPPCP_LIBRARIES}")
  
  # Helper functions
  add_subdirectory(misc)
endif()

# HE_QAT Library
add_subdirectory(he_qat)

if(HE_QAT_TEST)
  include(cmake/gtest.cmake)
endif()
if(HE_QAT_BENCHMARK)
  include(cmake/gbenchmark.cmake)
endif()

#Validation test examples
if(HE_QAT_EXAMPLES)
   add_subdirectory(examples)
endif()

if(HE_QAT_TEST)
  add_subdirectory(test)
  add_custom_target(unittest COMMAND $<TARGET_FILE:he_qat_unittest> DEPENDS he_qat_unittest)
endif()

if(HE_QAT_BENCHMARK)
  add_subdirectory(benchmark)
  add_custom_target(benchmark COMMAND $<TARGET_FILE:he_qat_bench> DEPENDS he_qat_bench)
endif()

if(HE_QAT_DOCS)
  add_subdirectory(docs)
endif()



