# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

include(ExternalProject)
MESSAGE(STATUS "Configuring ipp-crypto")
set(IPPCRYPTO_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ext_ipp-crypto)
set(IPPCRYPTO_GIT_REPO_URL https://github.com/intel/ipp-crypto.git)
set(IPPCRYPTO_GIT_LABEL ippcp_2021.6)
set(IPPCRYPTO_SRC_DIR ${IPPCRYPTO_PREFIX}/src/ext_ipp-crypto/)

set(IPPCRYPTO_CXX_FLAGS "${IPCL_FORWARD_CMAKE_ARGS} -DNONPIC_LIB:BOOL=off -DMERGED_BLD:BOOL=on")

set(IPPCRYPTO_ARCH intel64)
set(BUILD_x64 ON)
if(BUILD_x64)
  if(NOT ${BUILD_x64})
    set(IPPCRYPTO_ARCH ia32)
  endif()
endif()

ExternalProject_Add(
  ext_ipp-crypto
  GIT_REPOSITORY ${IPPCRYPTO_GIT_REPO_URL}
  GIT_TAG ${IPPCRYPTO_GIT_LABEL}
  PREFIX ${IPPCRYPTO_PREFIX}
  INSTALL_DIR ${IPPCRYPTO_PREFIX}
  CMAKE_ARGS ${IPPCRYPTO_CXX_FLAGS}
             -DCMAKE_INSTALL_PREFIX=${IPPCRYPTO_PREFIX}
             -DARCH=${IPPCRYPTO_ARCH}
             -DCMAKE_ASM_NASM_COMPILER=nasm
             -DCMAKE_BUILD_TYPE=Release
  UPDATE_COMMAND ""
)

set(IPPCRYPTO_INC_DIR ${IPPCRYPTO_PREFIX}/include)

if(IPCL_SHARED)
  add_library(libippcrypto INTERFACE)
  add_dependencies(libippcrypto ext_ipp-crypto)

  ExternalProject_Get_Property(ext_ipp-crypto SOURCE_DIR BINARY_DIR)

  target_link_libraries(libippcrypto INTERFACE ${IPPCRYPTO_PREFIX}/lib/${IPPCRYPTO_ARCH}/libippcp.a ${IPPCRYPTO_PREFIX}/lib/${IPPCRYPTO_ARCH}/libcrypto_mb.a)
  target_include_directories(libippcrypto SYSTEM INTERFACE ${IPPCRYPTO_PREFIX}/include)

else()

  add_library(libippcrypto::ippcp STATIC IMPORTED GLOBAL)
  add_library(libippcrypto::crypto_mb STATIC IMPORTED GLOBAL)

  add_dependencies(libippcrypto::ippcp ext_ipp-crypto)
  add_dependencies(libippcrypto::crypto_mb ext_ipp-crypto)

  find_package(OpenSSL REQUIRED)

  set_target_properties(libippcrypto::ippcp PROPERTIES
            IMPORTED_LOCATION ${IPPCRYPTO_PREFIX}/lib/${IPPCRYPTO_ARCH}/libippcp.a
            INCLUDE_DIRECTORIES ${IPPCRYPTO_INC_DIR}
  )

  set_target_properties(libippcrypto::crypto_mb PROPERTIES
            IMPORTED_LOCATION ${IPPCRYPTO_PREFIX}/lib/${IPPCRYPTO_ARCH}/libcrypto_mb.a
            INCLUDE_DIRECTORIES ${IPPCRYPTO_INC_DIR}
  )
endif()
