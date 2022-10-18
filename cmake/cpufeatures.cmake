# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

include(ExternalProject)

message(STATUS "configuring cpu_features")
set(CPUFEATURES_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ext_cpufeatures)
set(CPUFEATURES_DESTDIR ${CPUFEATURES_PREFIX}/cpufeatures_install)
set(CPUFEATURES_GIT_REPO_URL https://github.com/google/cpu_features.git)
set(CPUFEATURES_GIT_LABEL v0.7.0)
set(CPUFEATURES_C_FLAGS "${IPCL_FORWARD_CMAKE_ARGS} -fPIC")

ExternalProject_Add(
  ext_cpufeatures
  PREFIX ${CPUFEATURES_PREFIX}
  GIT_REPOSITORY ${CPUFEATURES_GIT_REPO_URL}
  GIT_TAG ${CPUFEATURES_GIT_LABEL}
  CMAKE_ARGS ${CPUFEATURES_C_FLAGS}
            -DCMAKE_BUILD_TYPE=Release
            -DBUILD_TESTING=OFF
            -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
            -DCMAKE_INSTALL_LIBDIR=lib
  UPDATE_COMMAND ""
  EXCLUDE_FROM_ALL TRUE
  INSTALL_COMMAND make DESTDIR=${CPUFEATURES_DESTDIR} install
  )


set(CPUFEATURES_INC_DIR ${CPUFEATURES_DESTDIR}/${CMAKE_INSTALL_PREFIX}/include)
set(CPUFEATURES_LIB_DIR ${CPUFEATURES_DESTDIR}/${CMAKE_INSTALL_PREFIX}/lib)

if(IPCL_SHARED)
  add_library(libcpu_features INTERFACE)
  add_dependencies(libcpu_features ext_cpufeatures)

  target_include_directories(libcpu_features SYSTEM
                            INTERFACE ${CPUFEATURES_INC_DIR})
  target_link_libraries(libcpu_features
                        INTERFACE ${CPUFEATURES_LIB_DIR}/libcpu_features.a)

  install(
    DIRECTORY ${CPUFEATURES_LIB_DIR}/
    DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/cpufeatures"
    USE_SOURCE_PERMISSIONS
  )
else()
  add_library(libcpu_features STATIC IMPORTED GLOBAL)
  add_dependencies(libcpu_features ext_cpufeatures)

  set_target_properties(libcpu_features PROPERTIES
      IMPORTED_LOCATION ${CPUFEATURES_LIB_DIR}/libcpu_features.a
      INCLUDE_DIRECTORIES ${CPUFEATURES_INC_DIR})
endif()
