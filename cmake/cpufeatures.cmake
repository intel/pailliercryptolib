# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

include(ExternalProject)
include(GNUInstallDirs)

set(CPUFEATURES_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ext_cpufeatures)
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
            -DCMAKE_INSTALL_PREFIX=${CPUFEATURES_PREFIX}
  UPDATE_COMMAND ""
  EXCLUDE_FROM_ALL TRUE)


set(CPUFEATURES_INC_DIR ${CPUFEATURES_PREFIX}/include)

if(IPCL_SHARED)
  add_library(libcpu_features INTERFACE)
  add_dependencies(libcpu_features ext_cpufeatures)

  target_include_directories(libcpu_features SYSTEM
                            INTERFACE ${CPUFEATURES_PREFIX}/include)
  target_link_libraries(libcpu_features
                        INTERFACE ${CPUFEATURES_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libcpu_features.a)

else()
  add_library(libcpu_features STATIC IMPORTED GLOBAL)
  add_dependencies(libcpu_features ext_cpufeatures)

  set_target_properties(libcpu_features PROPERTIES
      IMPORTED_LOCATION ${CPUFEATURES_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libcpu_features.a
      INCLUDE_DIRECTORIES ${CPUFEATURES_PREFIX}/include)
endif()
