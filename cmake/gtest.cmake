# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

include(ExternalProject)

set(GTEST_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ext_gtest)
set(GTEST_GIT_REPO_URL https://github.com/google/googletest.git)
set(GTEST_GIT_LABEL release-1.10.0)
set(GTEST_CXX_FLAGS "${IPCL_FORWARD_CMAKE_ARGS} -fPIC")

ExternalProject_Add(
  ext_gtest
  PREFIX ${GTEST_PREFIX}
  GIT_REPOSITORY ${GTEST_GIT_REPO_URL}
  GIT_TAG ${GTEST_GIT_LABEL}
  CMAKE_ARGS ${GTEST_CXX_FLAGS} -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -DCMAKE_INSTALL_LIBDIR=lib
  UPDATE_COMMAND ""
  EXCLUDE_FROM_ALL TRUE
  INSTALL_COMMAND ""
)

# install(
#   DIRECTORY ${GTEST_DESTDIR}/${CMAKE_INSTALL_PREFIX}/
#   DESTINATION "."
#   USE_SOURCE_PERMISSIONS
# )

ExternalProject_Get_Property(ext_gtest SOURCE_DIR BINARY_DIR)

add_library(libgtest INTERFACE)
add_dependencies(libgtest ext_gtest)

target_include_directories(libgtest SYSTEM
                           INTERFACE ${SOURCE_DIR}/googletest/include)
target_link_libraries(libgtest
                      INTERFACE ${BINARY_DIR}/lib/libgtest.a)
