# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

set(IPCL_BENCH_SRC main.cpp
  bench_cryptography.cpp
  bench_ops.cpp
  bench_cryptography_omp.cpp
  bench_ops_omp.cpp)

add_executable(bench_ipcl ${IPCL_BENCH_SRC})
target_include_directories(bench_ipcl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IPCL_INC_DIR}
)

target_link_libraries(bench_ipcl PRIVATE
  ipcl -pthread libgbenchmark
)

# enable OpenMP benchmark
if(IPCL_BENCHMARK_ENABLE_OMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries(bench_ipcl PRIVATE OpenMP::OpenMP_CXX)
  add_compile_definitions(IPCL_BENCHMARK_USE_OMP)
endif()
