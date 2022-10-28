// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "benchmark/benchmark.h"
#include "ipcl/context.hpp"

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
