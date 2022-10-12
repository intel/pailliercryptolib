// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <ipcl/context.hpp>
#include <gtest/gtest.h>

#include <random>

int main(int argc, char** argv) {
#ifdef IPCL_USE_QAT
  ipcl::initializeContext("QAT");
#else
  ipcl::initializeContext("default");
#endif  // IPCL_USE_QAT

  // Use system clock for seed
  srand(time(nullptr));
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();

  ipcl::terminateContext();

  return status;
}
