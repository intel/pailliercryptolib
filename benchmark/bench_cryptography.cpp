// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <vector>
#ifdef IPCL_USE_OMP
#include <omp.h>
#endif  // IPCL_USE_OMP

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

  std::vector<std::vector<ipcl::BigNumber>> pt(dsize,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> ct(dsize,
                                               std::vector<ipcl::BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i)
    pt[i][0] = ipcl::BigNumber((unsigned int)(i * 1024) + 999);

  for (auto _ : state) {
    for (size_t i = 0; i < dsize; ++i) key.pub_key->encrypt(ct[i], pt[i]);
  }
}

BENCHMARK(BM_Encrypt)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64});

static void BM_Encrypt_buff8(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<ipcl::BigNumber>> pt(dsize / 8,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> ct(dsize / 8,
                                               std::vector<ipcl::BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i)
    pt[i / 8][i & 7] = ipcl::BigNumber((unsigned int)(i * 1024) + 999);

  for (auto _ : state) {
    for (size_t i = 0; i < dsize / 8; ++i) key.pub_key->encrypt(ct[i], pt[i]);
  }
}

BENCHMARK(BM_Encrypt_buff8)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Decrypt(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<ipcl::BigNumber>> pt(dsize,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> ct(dsize,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> de_ct(
      dsize, std::vector<ipcl::BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i)
    pt[i][0] = ipcl::BigNumber((unsigned int)(i * 1024) + 999);

  for (size_t i = 0; i < dsize; ++i) key.pub_key->encrypt(ct[i], pt[i]);

  for (auto _ : state) {
    for (size_t i = 0; i < dsize; ++i) {
      key.priv_key->decrypt(de_ct[i], ct[i]);
    }
  }
}

BENCHMARK(BM_Decrypt)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64});

static void BM_Decrypt_buff8(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<ipcl::BigNumber>> pt(dsize / 8,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> ct(dsize / 8,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> de_ct(
      dsize / 8, std::vector<ipcl::BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i)
    pt[i / 8][i & 7] = ipcl::BigNumber((unsigned int)(i * 1024) + 999);

  for (size_t i = 0; i < dsize / 8; ++i) key.pub_key->encrypt(ct[i], pt[i]);

  for (auto _ : state) {
    for (size_t i = 0; i < dsize / 8; ++i)
      key.priv_key->decrypt(de_ct[i], ct[i]);
  }
}

BENCHMARK(BM_Decrypt_buff8)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

#ifdef IPCL_USE_OMP
static void BM_Encrypt_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<ipcl::BigNumber>> pt(dsize,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> ct(dsize,
                                               std::vector<ipcl::BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i)
    pt[i][0] = ipcl::BigNumber((unsigned int)(i * 1024) + 999);

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize; ++i) key.pub_key->encrypt(ct[i], pt[i]);
  }
}
BENCHMARK(BM_Encrypt_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Encrypt_buff8_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<ipcl::BigNumber>> pt(dsize / 8,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> ct(dsize / 8,
                                               std::vector<ipcl::BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i)
    pt[i / 8][i & 7] = ipcl::BigNumber((unsigned int)(i * 1024) + 999);

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize / 8; ++i) key.pub_key->encrypt(ct[i], pt[i]);
  }
}

BENCHMARK(BM_Encrypt_buff8_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Decrypt_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<ipcl::BigNumber>> pt(dsize,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> ct(dsize,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> de_ct(
      dsize, std::vector<ipcl::BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i)
    pt[i][0] = ipcl::BigNumber((unsigned int)(i * 1024) + 999);

  for (size_t i = 0; i < dsize; ++i) key.pub_key->encrypt(ct[i], pt[i]);

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize; ++i) {
      key.priv_key->decrypt(de_ct[i], ct[i]);
    }
  }
}

BENCHMARK(BM_Decrypt_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Decrypt_buff8_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<ipcl::BigNumber>> pt(dsize / 8,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> ct(dsize / 8,
                                               std::vector<ipcl::BigNumber>(8));
  std::vector<std::vector<ipcl::BigNumber>> de_ct(
      dsize / 8, std::vector<ipcl::BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i)
    pt[i / 8][i & 7] = ipcl::BigNumber((unsigned int)(i * 1024) + 999);

  for (size_t i = 0; i < dsize / 8; ++i) key.pub_key->encrypt(ct[i], pt[i]);

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize / 8; ++i)
      key.priv_key->decrypt(de_ct[i], ct[i]);
  }
}

BENCHMARK(BM_Decrypt_buff8_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});
#endif  // IPCL_USE_OMP
