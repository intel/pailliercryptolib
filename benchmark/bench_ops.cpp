// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <vector>
#ifdef IPCL_USE_OMP
#include <omp.h>
#endif  // IPCL_USE_OMP
#include <iostream>

#include "ipcl/keygen.hpp"
#include "ipcl/ops.hpp"

static void BM_Add_CTCT(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_b(dsize, std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i) {
    a[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_a[i], a[i]);

    b[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_b[i], b[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber pen_b(key.pub_key, ct_b[i]);
      ipcl::EncryptedNumber sum = pen_a + pen_b;
    }
  }
}

BENCHMARK(BM_Add_CTCT)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64});

static void BM_Add_CTCT_buff8(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize / 8,
                                           std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_b(dsize / 8,
                                           std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize / 8; ++i) {
    for (size_t j = 0; j < 8; ++j) {
      a[i][j] = BigNumber((unsigned int)(i * 8 + j));
      b[i][j] = BigNumber((unsigned int)(i * 8 + j));
    }

    key.pub_key->encrypt(ct_a[i], a[i]);
    key.pub_key->encrypt(ct_b[i], b[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize / 8; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber pen_b(key.pub_key, ct_b[i]);
      ipcl::EncryptedNumber sum = pen_a + pen_b;
    }
  }
}

BENCHMARK(BM_Add_CTCT_buff8)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Add_CTPT(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize, std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i) {
    a[i][0] = BigNumber((unsigned int)i);
    b[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber sum = pen_a + b[i];
    }
  }
}

BENCHMARK(BM_Add_CTPT)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64});

static void BM_Add_CTPT_buff8(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize / 8,
                                           std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize / 8; ++i) {
    for (size_t j = 0; j < 8; ++j) {
      a[i][j] = BigNumber((unsigned int)(i * 8 + j));
      b[i][j] = BigNumber((unsigned int)(i * 8 + j));
    }

    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize / 8; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber sum = pen_a + b[i];
    }
  }
}

BENCHMARK(BM_Add_CTPT_buff8)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Mul_CTPT(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize, std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i) {
    a[i][0] = BigNumber((unsigned int)i);
    b[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber pen_b(key.pub_key, b[i]);
      ipcl::EncryptedNumber sum = pen_a * pen_b;
    }
  }
}

BENCHMARK(BM_Mul_CTPT)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64});

static void BM_Mul_CTPT_buff8(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize / 8,
                                           std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize / 8; ++i) {
    for (size_t j = 0; j < 8; ++j) {
      a[i][j] = BigNumber((unsigned int)(i * 8 + j));
      b[i][j] = BigNumber((unsigned int)(i * 8 + j));
    }

    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize / 8; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber pen_b(key.pub_key, b[i]);
      ipcl::EncryptedNumber sum = pen_a * pen_b;
    }
  }
}

BENCHMARK(BM_Mul_CTPT_buff8)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

#ifdef IPCL_USE_OMP
static void BM_Add_CTCT_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_b(dsize, std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i) {
    a[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_a[i], a[i]);

    b[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_b[i], b[i]);
  }

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber pen_b(key.pub_key, ct_b[i]);
      ipcl::EncryptedNumber sum = pen_a + pen_b;
    }
  }
}

BENCHMARK(BM_Add_CTCT_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Add_CTCT_buff8_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize / 8,
                                           std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_b(dsize / 8,
                                           std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize / 8; ++i) {
    for (size_t j = 0; j < 8; ++j) {
      a[i][j] = BigNumber((unsigned int)(i * 8 + j));
      b[i][j] = BigNumber((unsigned int)(i * 8 + j));
    }

    key.pub_key->encrypt(ct_a[i], a[i]);
    key.pub_key->encrypt(ct_b[i], b[i]);
  }

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize / 8; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber pen_b(key.pub_key, ct_b[i]);
      ipcl::EncryptedNumber sum = pen_a + pen_b;
    }
  }
}

BENCHMARK(BM_Add_CTCT_buff8_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Add_CTPT_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize, std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i) {
    a[i][0] = BigNumber((unsigned int)i);
    b[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber sum = pen_a + b[i];
    }
  }
}

BENCHMARK(BM_Add_CTPT_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Add_CTPT_buff8_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize / 8,
                                           std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize / 8; ++i) {
    for (size_t j = 0; j < 8; ++j) {
      a[i][j] = BigNumber((unsigned int)(i * 8 + j));
      b[i][j] = BigNumber((unsigned int)(i * 8 + j));
    }

    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize / 8; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber sum = pen_a + b[i];
    }
  }
}

BENCHMARK(BM_Add_CTPT_buff8_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Mul_CTPT_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize, std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize; ++i) {
    a[i][0] = BigNumber((unsigned int)i);
    b[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber pen_b(key.pub_key, b[i]);
      ipcl::EncryptedNumber sum = pen_a * pen_b;
    }
  }
}

BENCHMARK(BM_Mul_CTPT_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Mul_CTPT_buff8_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<std::vector<BigNumber>> a(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> b(dsize / 8, std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> ct_a(dsize / 8,
                                           std::vector<BigNumber>(8));

  for (size_t i = 0; i < dsize / 8; ++i) {
    for (size_t j = 0; j < 8; ++j) {
      a[i][j] = BigNumber((unsigned int)(i * 8 + j));
      b[i][j] = BigNumber((unsigned int)(i * 8 + j));
    }

    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize / 8; ++i) {
      ipcl::EncryptedNumber pen_a(key.pub_key, ct_a[i]);
      ipcl::EncryptedNumber pen_b(key.pub_key, b[i]);
      ipcl::EncryptedNumber sum = pen_a * pen_b;
    }
  }
}

BENCHMARK(BM_Mul_CTPT_buff8_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});
#endif  // IPCL_USE_OMP
