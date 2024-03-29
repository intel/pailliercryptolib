# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# Unit tests
set(IPCL_UNITTEST_SRC
  main.cpp
  test_cryptography.cpp
  test_ops.cpp
  test_serialization.cpp
)

add_executable(unittest_ipcl ${IPCL_UNITTEST_SRC})
target_include_directories(unittest_ipcl PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${IPCL_INC_DIR}
)
target_link_libraries(unittest_ipcl PRIVATE
  ipcl -pthread libgtest
)

if(IPCL_SHARED)
  target_link_libraries(unittest_ipcl PRIVATE IPPCP::ippcp)
endif()

find_package(OpenMP REQUIRED)
target_link_libraries(ipcl PUBLIC OpenMP::OpenMP_CXX)
