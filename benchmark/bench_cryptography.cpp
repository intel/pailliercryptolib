// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <iostream>
#include <vector>

#include "ipcl/keygen.hpp"

static void BM_KeyGen(benchmark::State& state) {
  int64_t n_length = state.range(0);
  for (auto _ : state) {
    ipcl::keyPair key = ipcl::generateKeypair(n_length, true);
  }
}
BENCHMARK(BM_KeyGen)->Unit(benchmark::kMicrosecond)->Args({1024})->Args({2048});

static void BM_Encrypt(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<uint32_t> exp_value_v(dsize);

  for (size_t i = 0; i < dsize; i++)
    exp_value_v[i] = (unsigned int)(i * 1024) + 999;

  ipcl::PlainText pt(exp_value_v);
  ipcl::CipherText ct;

  for (auto _ : state) ct = key.pub_key->encrypt(pt);
}

BENCHMARK(BM_Encrypt)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64})->Args({128})->Args({256})->Args({512})->Args({1024})->Args({2048})->Args({2100});

static void BM_Decrypt(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<uint32_t> exp_value_v(dsize);

  for (size_t i = 0; i < dsize; i++)
    exp_value_v[i] = (unsigned int)(i * 1024) + 999;

  ipcl::PlainText pt(exp_value_v), dt;
  ipcl::CipherText ct = key.pub_key->encrypt(pt);

  for (auto _ : state) dt = key.priv_key->decrypt(ct);
}

BENCHMARK(BM_Decrypt)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64})->Args({128})->Args({256})->Args({512})->Args({1024})->Args({2048})->Args({2100});
