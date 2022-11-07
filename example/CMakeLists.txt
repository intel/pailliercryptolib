# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15.1)

project(ipcl_example LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(IPCL 2.0.0 REQUIRED HINTS ${IPCL_HINT_DIR})

set(examples encrypt_decrypt add_mul hybridmode)
foreach(ex IN LISTS examples)
  add_executable(example_${ex} example_${ex}.cpp)
  target_link_libraries(example_${ex} PRIVATE IPCL::ipcl)
endforeach()
