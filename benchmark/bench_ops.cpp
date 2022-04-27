// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <iostream>
#include <vector>

#include "ipcl/ciphertext.hpp"
#include "ipcl/keygen.hpp"
#include "ipcl/plaintext.hpp"

static void BM_Add_CTCT(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<uint32_t> exp_value1_v(dsize, 65535), exp_value2_v(dsize, 65535);

  ipcl::PlainText pt1, pt2;
  for (int i = 0; i < dsize; i++) {
    exp_value1_v[i] += (unsigned int)(i * 1024);
    exp_value2_v[i] += (unsigned int)(i * 1024);
  }
  pt1 = ipcl::PlainText(exp_value1_v);
  pt2 = ipcl::PlainText(exp_value2_v);

  ipcl::CipherText ct1 = key.pub_key->encrypt(pt1);
  ipcl::CipherText ct2 = key.pub_key->encrypt(pt2);
  ipcl::CipherText sum;
  for (auto _ : state) sum = ct1 + ct2;
}

BENCHMARK(BM_Add_CTCT)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64});

static void BM_Add_CTPT(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<uint32_t> exp_value1_v(dsize, 65535), exp_value2_v(dsize, 65535);

  ipcl::PlainText pt1, pt2;
  for (int i = 0; i < dsize; i++) {
    exp_value1_v[i] += (unsigned int)(i * 1024);
    exp_value2_v[i] += (unsigned int)(i * 1024);
  }
  pt1 = ipcl::PlainText(exp_value1_v);
  pt2 = ipcl::PlainText(exp_value2_v);

  ipcl::CipherText ct1 = key.pub_key->encrypt(pt1);
  ipcl::CipherText sum;

  for (auto _ : state) sum = ct1 + pt2;
}

BENCHMARK(BM_Add_CTPT)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64});

static void BM_Mul_CTPT(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<uint32_t> exp_value1_v(dsize, 65535), exp_value2_v(dsize, 65535);

  ipcl::PlainText pt1, pt2;
  for (int i = 0; i < dsize; i++) {
    exp_value1_v[i] += (unsigned int)(i * 1024);
    exp_value2_v[i] += (unsigned int)(i * 1024);
  }
  pt1 = ipcl::PlainText(exp_value1_v);
  pt2 = ipcl::PlainText(exp_value2_v);

  ipcl::CipherText ct1 = key.pub_key->encrypt(pt1);
  ipcl::CipherText product;

  for (auto _ : state) product = ct1 * pt2;
}

BENCHMARK(BM_Mul_CTPT)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64});
