# Copyright (C) 2021-2022 Intel Corporation

include(ExternalProject)
MESSAGE(STATUS "Configuring ipp-crypto")
set(IPPCRYPTO_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ext_ipp-crypto)
set(IPPCRYPTO_GIT_REPO_URL https://github.com/intel/ipp-crypto.git)
set(IPPCRYPTO_GIT_LABEL ipp-crypto_2021_4)
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

file(MAKE_DIRECTORY ${IPPCRYPTO_PREFIX}/include)
set(IPPCRYPTO_INC_DIR ${IPPCRYPTO_PREFIX}/include)
ExternalProject_Get_Property(ext_ipp-crypto SOURCE_DIR BINARY_DIR)


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

add_library(libippcrypto INTERFACE IMPORTED)
set_property(TARGET libippcrypto PROPERTY INTERFACE_LINK_LIBRARIES libippcrypto::ippcp libippcrypto::crypto_mb)
target_include_directories(libippcrypto SYSTEM INTERFACE ${IPPCRYPTO_INC_DIR})
target_link_libraries(libippcrypto INTERFACE OpenSSL::SSL OpenSSL::Crypto)

add_dependencies(libippcrypto ext_ipp-crypto)
