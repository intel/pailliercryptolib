# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

include(ExternalProject)

set(GBENCHMARK_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ext_gbenchmark)

set(GBENCHMARK_SRC_DIR ${GBENCHMARK_PREFIX}/src/ext_gbenchmark/)
set(GBENCHMARK_BUILD_DIR ${GBENCHMARK_PREFIX}/src/ext_gbenchmark-build/)
set(GBENCHMARK_REPO_URL https://github.com/google/benchmark.git)
set(GBENCHMARK_GIT_TAG v1.5.6)
set(GBENCHMARK_CXX_FLAGS "${IPCL_FORWARD_CMAKE_ARGS} -fPIC")

set(GBENCHMARK_PATHS ${GBENCHMARK_SRC_DIR} ${GBENCHMARK_BUILD_DIR}/src/libbenchmark.a)

ExternalProject_Add(
  ext_gbenchmark
  GIT_REPOSITORY ${GBENCHMARK_REPO_URL}
  GIT_TAG ${GBENCHMARK_GIT_TAG}
  PREFIX  ${GBENCHMARK_PREFIX}
  CMAKE_ARGS ${GBENCHMARK_CXX_FLAGS}
             -DCMAKE_INSTALL_PREFIX=${GBENCHMARK_PREFIX}
             -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
             -DBENCHMARK_ENABLE_TESTING=OFF
             -DCMAKE_BUILD_TYPE=Release
  BUILD_BYPRODUCTS ${GBENCHMARK_PATHS}
  # Skip updates
  UPDATE_COMMAND ""
  EXCLUDE_FROM_ALL TRUE
)

add_library(libgbenchmark INTERFACE)
add_dependencies(libgbenchmark ext_gbenchmark)

message(STATUS "DEBUG -- libdir: ${CMAKE_INSTALL_LIBDIR}")
ExternalProject_Get_Property(ext_gbenchmark SOURCE_DIR BINARY_DIR)
file(STRINGS /etc/os-release LINUX_ID REGEX "^ID=")
string(REGEX REPLACE "ID=\(.*)" "\\1" LINUX_ID "${LINUX_ID}")
if(${LINUX_ID} STREQUAL "ubuntu")
  target_link_libraries(libgbenchmark INTERFACE ${GBENCHMARK_PREFIX}/lib/libbenchmark.a)
else()
  # non debian systems install gbenchmark lib under lib64
  target_link_libraries(libgbenchmark INTERFACE ${GBENCHMARK_PREFIX}/lib64/libbenchmark.a)
endif()

target_include_directories(libgbenchmark SYSTEM
                                    INTERFACE ${GBENCHMARK_PREFIX}/include)
