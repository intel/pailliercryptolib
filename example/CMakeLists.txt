# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15.1)

project(ipcl_example LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(IPCL 1.1.4 REQUIRED HINTS ${IPCL_HINT_DIR})

add_executable(test test.cpp)
target_link_libraries(test PRIVATE IPCL::ipcl)
