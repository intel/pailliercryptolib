# Copyright (C) 2021-2022 Intel Corporation

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
)

add_library(libgbenchmark INTERFACE)
add_dependencies(libgbenchmark ext_gbenchmark)

ExternalProject_Get_Property(ext_gbenchmark SOURCE_DIR BINARY_DIR)
if(EXISTS ${GBENCHMARK_PREFIX}/lib/libbenchmark.a)
  target_link_libraries(libgbenchmark INTERFACE ${GBENCHMARK_PREFIX}/lib/libbenchmark.a)
elseif(EXISTS ${GBENCHMARK_PREFIX}/lib64/libbenchmark.a)
  target_link_libraries(libgbenchmark INTERFACE ${GBENCHMARK_PREFIX}/lib64/libbenchmark.a)
else()
  message(FATAL "gBenchmark build failed")
endif()

target_include_directories(libgbenchmark SYSTEM
                                    INTERFACE ${GBENCHMARK_PREFIX}/include)
