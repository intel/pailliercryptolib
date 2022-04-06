// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifdef IPCL_USE_QAT
#include "he_qat_context.h"
#endif  // IPCL_USE_QAT

#include <benchmark/benchmark.h>

int main(int argc, char** argv) {
#ifdef IPCL_USE_QAT
  acquire_qat_devices();
#endif  // IPCL_USE_QAT

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

#ifdef IPCL_USE_QAT
  release_qat_devices();
#endif

  return 0;
}
