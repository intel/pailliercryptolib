# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

include(ExternalProject)
message(STATUS "Configuring ipp-crypto")

if(IPCL_IPPCRYPTO_PATH)
  if(IPCL_SHARED)
    set(IPPCP_SHARED ON)
  else()
    set(IPPCP_SHARED OFF)
  endif()

  find_package(ippcp 11.4 HINTS ${IPCL_IPPCRYPTO_PATH}/lib/cmake/ippcp)
endif()


if(ippcp_FOUND)
  message(STATUS "------------------ IPPCRYPTO FOUND!")
  get_target_property(IPPCRYPTO_INC_DIR IPPCP::ippcp INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(IPPCRYPTO_IMPORTED_LOCATION IPPCP::ippcp IMPORTED_LOCATION)
  get_filename_component(IPPCRYPTO_LIB_DIR ${IPPCRYPTO_IMPORTED_LOCATION} DIRECTORY)

  message(STATUS "------------------ IPPCRYPTO_INC_DIR: ${IPPCRYPTO_INC_DIR}")
  message(STATUS "------------------ IPPCRYPTO_LIB_DIR: ${IPPCRYPTO_LIB_DIR}")
else()

  set(IPPCRYPTO_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ext_ipp-crypto)
  set(IPPCRYPTO_DESTDIR ${IPPCRYPTO_PREFIX}/ippcrypto_install)
  set(IPPCRYPTO_DEST_INCLUDE_DIR include/ippcrypto)
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
              -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
              -DCMAKE_INSTALL_INCLUDEDIR=${IPPCRYPTO_DEST_INCLUDE_DIR}
    UPDATE_COMMAND ""
    PATCH_COMMAND git apply ${CMAKE_CURRENT_LIST_DIR}/patch/ippcrypto_patch.patch
    INSTALL_COMMAND make DESTDIR=${IPPCRYPTO_DESTDIR} install
  )

  set(IPPCRYPTO_INC_DIR ${IPPCRYPTO_DESTDIR}/${CMAKE_INSTALL_PREFIX}/include)
  set(IPPCRYPTO_LIB_DIR ${IPPCRYPTO_DESTDIR}/${CMAKE_INSTALL_PREFIX}/lib/${IPPCRYPTO_ARCH})
  if(IPCL_SHARED)
    # add_library(IPPCP INTERFACE)
    # add_library(IPPCP::ippcp ALIAS IPPCP)

    # add_dependencies(IPPCP ext_ipp-crypto)

    # target_include_directories(IPPCP SYSTEM INTERFACE ${IPPCRYPTO_INC_DIR})

    # # if ipcl python build
    # if(IPCL_INTERNAL_PYTHON_BUILD)
    #   target_link_libraries(IPPCP INTERFACE
    #     ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ippcrypto/libippcp.so
    #     ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ippcrypto/libcrypto_mb.so
    #   )

    #   add_custom_command(TARGET ext_ipp-crypto
    #     POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy_directory ${IPPCRYPTO_LIB_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ippcrypto
    #   )
    # else()
    #   target_link_libraries(IPPCP INTERFACE
    #     ${IPPCRYPTO_LIB_DIR}/libippcp.so
    #     ${IPPCRYPTO_LIB_DIR}/libcrypto_mb.so
    #   )
    # endif()

    # install(
    #   DIRECTORY ${IPPCRYPTO_LIB_DIR}/
    #   DESTINATION "${IPCL_INSTALL_LIBDIR}/ippcrypto"
    #   USE_SOURCE_PERMISSIONS
    # )

    add_library(IPPCP_ippcp INTERFACE)
    add_library(IPPCP::ippcp ALIAS IPPCP_ippcp)

    add_library(IPPCP_crypto_mb INTERFACE)
    add_library(IPPCP::crypto_mb ALIAS IPPCP_crypto_mb)

    add_dependencies(IPPCP_ippcp ext_ipp-crypto)
    add_dependencies(IPPCP_crypto_mb ext_ipp-crypto)

    target_include_directories(IPPCP_ippcp SYSTEM INTERFACE ${IPPCRYPTO_INC_DIR})
    target_include_directories(IPPCP_crypto_mb SYSTEM INTERFACE ${IPPCRYPTO_INC_DIR})

    # if ipcl python build
    if(IPCL_INTERNAL_PYTHON_BUILD)
      target_link_libraries(IPPCP_ippcp INTERFACE
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ippcrypto/libippcp.so
      )
      target_link_libraries(IPPCP_crypto_mb INTERFACE
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ippcrypto/libcrypto_mb.so
      )

      add_custom_command(TARGET ext_ipp-crypto
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${IPPCRYPTO_LIB_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ippcrypto
      )
    else()
      target_link_libraries(IPPCP_ippcp INTERFACE
        ${IPPCRYPTO_LIB_DIR}/libippcp.so
      )
      target_link_libraries(IPPCP_crypto_mb INTERFACE
        ${IPPCRYPTO_LIB_DIR}/libcrypto_mb.so
      )

    endif()

    install(
      DIRECTORY ${IPPCRYPTO_LIB_DIR}/
      DESTINATION "${IPCL_INSTALL_LIBDIR}/ippcrypto"
      USE_SOURCE_PERMISSIONS
    )


  else()

    add_library(IPPCP::ippcp STATIC IMPORTED GLOBAL)
    add_library(IPPCP::crypto_mb STATIC IMPORTED GLOBAL)

    add_dependencies(IPPCP::ippcp ext_ipp-crypto)
    add_dependencies(IPPCP::crypto_mb ext_ipp-crypto)

    find_package(OpenSSL REQUIRED)

    set_target_properties(IPPCP::ippcp PROPERTIES
              IMPORTED_LOCATION ${IPPCRYPTO_LIB_DIR}/libippcp.a
              INCLUDE_DIRECTORIES ${IPPCRYPTO_INC_DIR}
    )

    set_target_properties(IPPCP::crypto_mb PROPERTIES
              IMPORTED_LOCATION ${IPPCRYPTO_LIB_DIR}/libcrypto_mb.a
              INCLUDE_DIRECTORIES ${IPPCRYPTO_INC_DIR}
    )
  endif()
endif()
