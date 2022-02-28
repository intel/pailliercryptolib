// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <vector>
#ifdef IPCL_BENCHMARK_OMP
#include <omp.h>
#endif
#include <iostream>

#include "ipcl/paillier_keygen.hpp"
#include "ipcl/paillier_ops.hpp"

static void BM_Add_CTCT(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize];
  BigNumber** b = new BigNumber*[dsize];
  BigNumber** ct_a = new BigNumber*[dsize];
  BigNumber** ct_b = new BigNumber*[dsize];

  for (size_t i = 0; i < dsize; ++i) {
    a[i] = new BigNumber[8];
    a[i][0] = BigNumber((unsigned int)i);
    ct_a[i] = new BigNumber[8];
    key.pub_key->encrypt(ct_a[i], a[i]);

    b[i] = new BigNumber[8];
    b[i][0] = BigNumber((unsigned int)i);
    ct_b[i] = new BigNumber[8];
    key.pub_key->encrypt(ct_b[i], b[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize; ++i) {
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber pen_b(key.pub_key, ct_b[i]);
      PaillierEncryptedNumber sum = pen_a + pen_b;
    }
  }

  for (size_t i = 0; i < dsize; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
    delete[] ct_b[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
  delete[] ct_b;
}

BENCHMARK(BM_Add_CTCT)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64});

static void BM_Add_CTCT_buff8(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize / 8];
  BigNumber** b = new BigNumber*[dsize / 8];
  BigNumber** ct_a = new BigNumber*[dsize / 8];
  BigNumber** ct_b = new BigNumber*[dsize / 8];

  for (size_t i = 0; i < dsize / 8; ++i) {
    a[i] = new BigNumber[8];
    ct_a[i] = new BigNumber[8];

    b[i] = new BigNumber[8];
    ct_b[i] = new BigNumber[8];
    for (size_t j = 0; j < 8; ++j) {
      a[i][j] = BigNumber((unsigned int)(i * 8 + j));
      b[i][j] = BigNumber((unsigned int)(i * 8 + j));
    }

    key.pub_key->encrypt(ct_a[i], a[i]);
    key.pub_key->encrypt(ct_b[i], b[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize / 8; ++i) {
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber pen_b(key.pub_key, ct_b[i]);
      PaillierEncryptedNumber sum = pen_a + pen_b;
    }
  }

  for (size_t i = 0; i < dsize / 8; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
    delete[] ct_b[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
  delete[] ct_b;
}

BENCHMARK(BM_Add_CTCT_buff8)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Add_CTPT(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize];
  BigNumber** ct_a = new BigNumber*[dsize];
  BigNumber** b = new BigNumber*[dsize];

  for (size_t i = 0; i < dsize; ++i) {
    a[i] = new BigNumber[8];
    a[i][0] = BigNumber((unsigned int)i);
    ct_a[i] = new BigNumber[8];
    b[i] = new BigNumber[8];
    b[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize; ++i) {
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber sum = pen_a + b[i];
    }
  }

  for (size_t i = 0; i < dsize; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
}

BENCHMARK(BM_Add_CTPT)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64});

static void BM_Add_CTPT_buff8(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize / 8];
  BigNumber** b = new BigNumber*[dsize / 8];
  BigNumber** ct_a = new BigNumber*[dsize / 8];

  for (size_t i = 0; i < dsize / 8; ++i) {
    a[i] = new BigNumber[8];
    ct_a[i] = new BigNumber[8];

    b[i] = new BigNumber[8];
    for (size_t j = 0; j < 8; ++j) {
      a[i][j] = BigNumber((unsigned int)(i * 8 + j));
      b[i][j] = BigNumber((unsigned int)(i * 8 + j));
    }

    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize / 8; ++i) {
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber sum = pen_a + b[i];
    }
  }

  for (size_t i = 0; i < dsize / 8; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
}

BENCHMARK(BM_Add_CTPT_buff8)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Mul_CTPT(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize];
  BigNumber** ct_a = new BigNumber*[dsize];
  BigNumber** b = new BigNumber*[dsize];

  for (size_t i = 0; i < dsize; ++i) {
    a[i] = new BigNumber[8];
    a[i][0] = BigNumber((unsigned int)i);
    ct_a[i] = new BigNumber[8];
    b[i] = new BigNumber[8];
    b[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize; ++i) {
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber pen_b(key.pub_key, b[i]);
      PaillierEncryptedNumber sum = pen_a * pen_b;
    }
  }

  for (size_t i = 0; i < dsize; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
}

BENCHMARK(BM_Mul_CTPT)->Unit(benchmark::kMicrosecond)->Args({16})->Args({64});

static void BM_Mul_CTPT_buff8(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize / 8];
  BigNumber** b = new BigNumber*[dsize / 8];
  BigNumber** ct_a = new BigNumber*[dsize / 8];

  for (size_t i = 0; i < dsize / 8; ++i) {
    a[i] = new BigNumber[8];
    ct_a[i] = new BigNumber[8];

    b[i] = new BigNumber[8];
    for (size_t j = 0; j < 8; ++j) {
      a[i][j] = BigNumber((unsigned int)(i * 8 + j));
      b[i][j] = BigNumber((unsigned int)(i * 8 + j));
    }

    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
    for (size_t i = 0; i < dsize / 8; ++i) {
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber pen_b(key.pub_key, b[i]);
      PaillierEncryptedNumber sum = pen_a * pen_b;
    }
  }

  for (size_t i = 0; i < dsize / 8; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
}

BENCHMARK(BM_Mul_CTPT_buff8)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

#ifdef IPCL_BENCHMARK_OMP
static void BM_Add_CTCT_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize];
  BigNumber** b = new BigNumber*[dsize];
  BigNumber** ct_a = new BigNumber*[dsize];
  BigNumber** ct_b = new BigNumber*[dsize];

  for (size_t i = 0; i < dsize; ++i) {
    a[i] = new BigNumber[8];
    a[i][0] = BigNumber((unsigned int)i);
    ct_a[i] = new BigNumber[8];
    key.pub_key->encrypt(ct_a[i], a[i]);

    b[i] = new BigNumber[8];
    b[i][0] = BigNumber((unsigned int)i);
    ct_b[i] = new BigNumber[8];
    key.pub_key->encrypt(ct_b[i], b[i]);
  }

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize; ++i) {
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber pen_b(key.pub_key, ct_b[i]);
      PaillierEncryptedNumber sum = pen_a + pen_b;
    }
  }

  for (size_t i = 0; i < dsize; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
    delete[] ct_b[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
  delete[] ct_b;
}

BENCHMARK(BM_Add_CTCT_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Add_CTCT_buff8_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize / 8];
  BigNumber** b = new BigNumber*[dsize / 8];
  BigNumber** ct_a = new BigNumber*[dsize / 8];
  BigNumber** ct_b = new BigNumber*[dsize / 8];

  for (size_t i = 0; i < dsize / 8; ++i) {
    a[i] = new BigNumber[8];
    ct_a[i] = new BigNumber[8];

    b[i] = new BigNumber[8];
    ct_b[i] = new BigNumber[8];
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
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber pen_b(key.pub_key, ct_b[i]);
      PaillierEncryptedNumber sum = pen_a + pen_b;
    }
  }

  for (size_t i = 0; i < dsize / 8; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
    delete[] ct_b[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
  delete[] ct_b;
}

BENCHMARK(BM_Add_CTCT_buff8_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Add_CTPT_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize];
  BigNumber** ct_a = new BigNumber*[dsize];
  BigNumber** b = new BigNumber*[dsize];

  for (size_t i = 0; i < dsize; ++i) {
    a[i] = new BigNumber[8];
    a[i][0] = BigNumber((unsigned int)i);
    ct_a[i] = new BigNumber[8];
    b[i] = new BigNumber[8];
    b[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize; ++i) {
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber sum = pen_a + b[i];
    }
  }

  for (size_t i = 0; i < dsize; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
}

BENCHMARK(BM_Add_CTPT_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Add_CTPT_buff8_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize / 8];
  BigNumber** b = new BigNumber*[dsize / 8];
  BigNumber** ct_a = new BigNumber*[dsize / 8];

  for (size_t i = 0; i < dsize / 8; ++i) {
    a[i] = new BigNumber[8];
    ct_a[i] = new BigNumber[8];

    b[i] = new BigNumber[8];
    for (size_t j = 0; j < 8; ++j) {
      a[i][j] = BigNumber((unsigned int)(i * 8 + j));
      b[i][j] = BigNumber((unsigned int)(i * 8 + j));
    }

    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize / 8; ++i) {
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber sum = pen_a + b[i];
    }
  }

  for (size_t i = 0; i < dsize / 8; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
}

BENCHMARK(BM_Add_CTPT_buff8_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Mul_CTPT_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize];
  BigNumber** ct_a = new BigNumber*[dsize];
  BigNumber** b = new BigNumber*[dsize];

  for (size_t i = 0; i < dsize; ++i) {
    a[i] = new BigNumber[8];
    a[i][0] = BigNumber((unsigned int)i);
    ct_a[i] = new BigNumber[8];
    b[i] = new BigNumber[8];
    b[i][0] = BigNumber((unsigned int)i);
    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize; ++i) {
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber pen_b(key.pub_key, b[i]);
      PaillierEncryptedNumber sum = pen_a * pen_b;
    }
  }

  for (size_t i = 0; i < dsize; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
}

BENCHMARK(BM_Mul_CTPT_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});

static void BM_Mul_CTPT_buff8_OMP(benchmark::State& state) {
  size_t dsize = state.range(0);
  keyPair key = generateKeypair(2048, true);

  BigNumber** a = new BigNumber*[dsize / 8];
  BigNumber** b = new BigNumber*[dsize / 8];
  BigNumber** ct_a = new BigNumber*[dsize / 8];

  for (size_t i = 0; i < dsize / 8; ++i) {
    a[i] = new BigNumber[8];
    ct_a[i] = new BigNumber[8];

    b[i] = new BigNumber[8];
    for (size_t j = 0; j < 8; ++j) {
      a[i][j] = BigNumber((unsigned int)(i * 8 + j));
      b[i][j] = BigNumber((unsigned int)(i * 8 + j));
    }

    key.pub_key->encrypt(ct_a[i], a[i]);
  }

  for (auto _ : state) {
#pragma omp parallel for
    for (size_t i = 0; i < dsize / 8; ++i) {
      PaillierEncryptedNumber pen_a(key.pub_key, ct_a[i]);
      PaillierEncryptedNumber pen_b(key.pub_key, b[i]);
      PaillierEncryptedNumber sum = pen_a * pen_b;
    }
  }

  for (size_t i = 0; i < dsize / 8; ++i) {
    delete[] a[i];
    delete[] b[i];
    delete[] ct_a[i];
  }

  delete[] a;
  delete[] ct_a;
  delete[] b;
}

BENCHMARK(BM_Mul_CTPT_buff8_OMP)
    ->Unit(benchmark::kMicrosecond)
    ->Args({16})
    ->Args({64});
#endif
