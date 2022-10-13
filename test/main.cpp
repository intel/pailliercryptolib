// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <ipcl/context.hpp>
#include <gtest/gtest.h>

#include <random>

int main(int argc, char** argv) {
#ifdef IPCL_USE_QAT
  ipcl::initializeContext("QAT");
  
  if (ipcl::isQATActive())
     std::cout << "QAT Context: ACTIVE" << std::endl;
  else 
    std::cout << "Error: QAT Context INACTIVE." << std::endl;

  if (ipcl::isQATRunning())
     std::cout << "QAT Instances: RUNNING" << std::endl;
  else 
    std::cout << "Error: QAT Instances NOT RUNNING." << std::endl;
#else
  ipcl::initializeContext("default");
#endif  // IPCL_USE_QAT

  // Use system clock for seed
  srand(time(nullptr));
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();

  ipcl::terminateContext();

#ifdef IPCL_USE_QAT
  if (!ipcl::isQATActive())
    std::cout << "QAT Context: INACTIVE" << std::endl;
  else 
    std::cout << "Error: QAT Context ACTIVE." << std::endl;
  if (!ipcl::isQATRunning())
    std::cout << "QAT Instances: NOT RUNNING" << std::endl;
  else
    std::cout << "Error: QAT Instances STILL RUNNING." << std::endl;
#endif

  return status;
}
