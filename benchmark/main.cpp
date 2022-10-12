// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <ipcl/context.hpp>
#include <benchmark/benchmark.h>

int main(int argc, char** argv) {
#ifdef IPCL_USE_QAT
  ipcl::initializeContext("QAT");
#else
  ipcl::initializeContext("default");
#endif  // IPCL_USE_QAT

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  
  ipcl::terminateContext();

  return 0;
}
