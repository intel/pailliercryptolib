# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# Unit tests
set(IPCL_UNITTEST_SRC main.cpp
test_cryptography.cpp
test_ops.cpp
test_cryptography_omp.cpp
test_ops_omp.cpp)

add_executable(unittest_ipcl ${IPCL_UNITTEST_SRC})
target_include_directories(unittest_ipcl PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${IPCL_INC_DIR}
)
target_link_libraries(unittest_ipcl PRIVATE
  ipcl -pthread libgtest
)

# enable OpenMP unittests
if(IPCL_TEST_ENABLE_OMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries(unittest_ipcl PRIVATE OpenMP::OpenMP_CXX)
  add_compile_definitions(IPCL_TEST_USE_OMP)
endif()
