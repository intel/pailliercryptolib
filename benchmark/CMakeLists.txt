# Copyright (C) 2021-2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

set(SRC main.cpp
  bench_cryptography.cpp
  bench_ops.cpp)

add_executable(bench_ipcl ${SRC})
target_include_directories(bench_ipcl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IPCL_INC_DIR}
)

target_link_libraries(bench_ipcl PRIVATE
  ipcl libgbenchmark Threads::Threads
)

if(IPCL_TEST_OMP)
  find_package(OpenMP REQUIRED)
  add_compile_definitions(BENCHMARK_OMP)
  target_link_libraries(bench_ipcl PRIVATE OpenMP::OpenMP_CXX)
endif()
