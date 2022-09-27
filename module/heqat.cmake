# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

include(ExternalProject)
MESSAGE(STATUS "Configuring HE QAT")
set(HEQAT_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ext_he_qat)
set(HEQAT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/module/heqat)
set(HEQAT_CXX_FLAGS "${IPCL_FORWARD_CMAKE_ARGS}")

ExternalProject_Add(
  ext_he_qat
  SOURCE_DIR ${HEQAT_SRC_DIR}
  PREFIX ${HEQAT_PREFIX}
  INSTALL_DIR ${HEQAT_PREFIX}
  CMAKE_ARGS ${HEQAT_CXX_FLAGS}
             -DCMAKE_INSTALL_PREFIX=${HEQAT_PREFIX}
	     -DHE_QAT_MISC=OFF
	     -DHE_QAT_DOCS=${IPCL_DOCS}
	     -DIPPCP_PREFIX_PATH=${IPPCRYPTO_PREFIX}/lib/cmake
             -DHE_QAT_SHARED=${IPCL_SHARED}
	     -DCMAKE_BUILD_TYPE=Release
  UPDATE_COMMAND ""
)
add_dependencies(ext_he_qat ext_ipp-crypto)

set(HEQAT_INC_DIR ${HEQAT_PREFIX}/include)

# Bring up CPA variables
include(${HEQAT_SRC_DIR}/icp/CMakeLists.txt)
list(APPEND HEQAT_INC_DIR ${ICP_INC_DIR})

if(IPCL_SHARED)
  add_library(libhe_qat INTERFACE)
  add_dependencies(libhe_qat ext_he_qat)

  ExternalProject_Get_Property(ext_he_qat SOURCE_DIR BINARY_DIR)

  target_link_libraries(libhe_qat INTERFACE ${HEQAT_PREFIX}/lib/libcpa_sample_utils.so)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(libhe_qat INTERFACE ${HEQAT_PREFIX}/lib/libhe_qat_debug.so)
  else()
    target_link_libraries(libhe_qat INTERFACE ${HEQAT_PREFIX}/lib/libhe_qat.so)
  endif()
  target_include_directories(libhe_qat SYSTEM INTERFACE ${HEQAT_INC_DIR})
else()
  add_library(libhe_qat STATIC IMPORTED GLOBAL)
  add_dependencies(libhe_qat ext_he_qat)

  ExternalProject_Get_Property(ext_he_qat SOURCE_DIR BINARY_DIR)

  set_target_properties(libhe_qat PROPERTIES
    IMPORTED_LOCATION ${HEQAT_PREFIX}/lib/libhe_qat.a
    INCLUDE_DIRECTORIES ${HEQAT_INC_DIR})
endif()
