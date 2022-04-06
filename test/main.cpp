// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifdef IPCL_USE_QAT
#include <he_qat_context.h>
#endif  // IPCL_USE_QAT

#include <gtest/gtest.h>

#include <random>

int main(int argc, char** argv) {
#ifdef IPCL_USE_QAT
  // Initialize QAT context
  acquire_qat_devices();
#endif  // IPCL_USE_QAT

  // Use system clock for seed
  srand(time(nullptr));
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();

#ifdef IPCL_USE_QAT
  // Destroy QAT context
  release_qat_devices();
#endif

  return status;
}
