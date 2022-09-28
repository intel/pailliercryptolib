# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15.1)

project(ipcl_example LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (DEFINED ENV{IPCL_DIR})
  set(IPCL_DIR $ENV{IPCL_DIR})
else()
  set(IPCL_DIR /opt/intel/ipcl)
endif()

find_package(IPCL REQUIRED PATHS ${IPCL_DIR})

add_executable(test test.cpp)
target_link_libraries(test PRIVATE IPCL::ipcl)
