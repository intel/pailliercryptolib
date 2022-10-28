# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

include(ExternalProject)
MESSAGE(STATUS "Configuring HE QAT")
set(HEQAT_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ext_he_qat)
set(HEQAT_DESTDIR ${HEQAT_PREFIX}/heqat_install)
set(HEQAT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/module/heqat)
set(HEQAT_CXX_FLAGS "${IPCL_FORWARD_CMAKE_ARGS}")

set(HEQAT_BUILD_TYPE Release)
if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(HEQAT_BUILD_TYPE Debug)
endif()

ExternalProject_Add(
  ext_he_qat
  SOURCE_DIR ${HEQAT_SRC_DIR}
  PREFIX ${HEQAT_PREFIX}
  #  INSTALL_DIR ${HEQAT_DESTDIR}
  CMAKE_ARGS ${HEQAT_CXX_FLAGS}
  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
	     -DHE_QAT_MISC=OFF
	     -DHE_QAT_DOCS=${IPCL_DOCS}
	     -DHE_QAT_SHARED=${IPCL_SHARED}
       -DHE_QAT_SAMPLES=OFF
	     -DCMAKE_BUILD_TYPE=${HEQAT_BUILD_TYPE}
  UPDATE_COMMAND ""
  EXCLUDE_FROM_ALL TRUE
  INSTALL_COMMAND make DESTDIR=${HEQAT_DESTDIR} install
)
add_dependencies(ext_he_qat ext_ipp-crypto)

set(HEQAT_INC_DIR ${HEQAT_DESTDIR}/${CMAKE_INSTALL_PREFIX}/include)
set(HEQAT_LIB_DIR ${HEQAT_DESTDIR}/${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

# Create heqat library interface
if(IPCL_SHARED)
  add_library(libhe_qat INTERFACE)
  add_dependencies(libhe_qat ext_he_qat)

  ExternalProject_Get_Property(ext_he_qat SOURCE_DIR BINARY_DIR)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	  target_link_libraries(libhe_qat INTERFACE ${HEQAT_LIB_DIR}/libhe_qat_debug.so)
  else()
	  target_link_libraries(libhe_qat INTERFACE ${HEQAT_LIB_DIR}/libhe_qat.so)
  endif()
  target_include_directories(libhe_qat SYSTEM INTERFACE ${HEQAT_INC_DIR})

  install(
    DIRECTORY ${HEQAT_LIB_DIR}/
    DESTINATION "${IPCL_INSTALL_LIBDIR}/heqat"
    USE_SOURCE_PERMISSIONS
    PATTERN "cmake" EXCLUDE
  )
else()
  add_library(libhe_qat STATIC IMPORTED GLOBAL)
  add_dependencies(libhe_qat ext_he_qat)

  ExternalProject_Get_Property(ext_he_qat SOURCE_DIR BINARY_DIR)

  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(libhe_qat PROPERTIES
            IMPORTED_LOCATION ${HEQAT_LIB_DIR}/libhe_qat_debug.a
      INCLUDE_DIRECTORIES ${HEQAT_INC_DIR})
  else()
    set_target_properties(libhe_qat PROPERTIES
            IMPORTED_LOCATION ${HEQAT_LIB_DIR}/libhe_qat.a
      INCLUDE_DIRECTORIES ${HEQAT_INC_DIR})
  endif()
endif()
