// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <climits>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "ipcl/ciphertext.hpp"
#include "ipcl/keygen.hpp"
#include "ipcl/plaintext.hpp"

constexpr int SELF_DEF_NUM_VALUES = 20;
constexpr int BATCH_SIZE = ipcl::IPCL_CRYPTO_MB_SIZE;

TEST(CryptoTest_OMP, CryptoTest_OMP) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;
  std::size_t v_size = (num_values + BATCH_SIZE - 1) / BATCH_SIZE;
  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  // init expected value
  std::vector<uint32_t> exp_value(BATCH_SIZE);
  for (int i = 0; i < BATCH_SIZE; i++) {
    exp_value[i] = dist(rng);
  }

  // 1. init pt
  // 2. encrypt pt to ct
  // 3. decrypt ct to dt
  std::vector<ipcl::PlainText> pt_v(v_size), dt_v(v_size);
  std::vector<ipcl::CipherText> ct_v(v_size);

#ifdef IPCL_BENCHMARK_USE_OMP
#pragma omp parallel for
#endif  // IPCL_BENCHMARK_USE_OMP
  for (std::size_t i = 0; i < v_size; i++) {
    pt_v[i] = ipcl::PlainText(exp_value);
    ct_v[i] = key.pub_key->encrypt(pt_v[i]);
    dt_v[i] = key.priv_key->decrypt(ct_v[i]);
  }

  // verify result.(compare dt with expected value)
  for (std::size_t i = 0; i < v_size; i++) {
    for (std::size_t j = 0; j < BATCH_SIZE; j++) {
      std::vector<uint32_t> v = dt_v[i].getElementVec(j);
      EXPECT_EQ(v[0], exp_value[j]);
    }
  }

  delete key.pub_key;
  delete key.priv_key;
}
