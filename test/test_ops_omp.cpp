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

TEST(OperationTest_OMP, CtPlusCtTest_OMP) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;
  std::size_t v_size = (num_values + BATCH_SIZE - 1) % BATCH_SIZE;
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  std::vector<uint32_t> exp_value1(BATCH_SIZE), exp_value2(BATCH_SIZE);
  for (int i = 0; i < v_size; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = dist(rng);
  }

  std::vector<ipcl::PlainText> pt1_v(v_size), pt2_v(v_size), dt_sum_v(v_size);
  std::vector<ipcl::CipherText> ct1_v(v_size), ct2_v(v_size), ct_sum_v(v_size);

#ifdef IPCL_BENCHMARK_USE_OMP
#pragma omp parallel for
#endif  // IPCL_BENCHMARK_USE_OMP
  for (std::size_t i = 0; i < v_size; i++) {
    pt1_v[i] = ipcl::PlainText(exp_value1);
    pt2_v[i] = ipcl::PlainText(exp_value2);
    ct1_v[i] = key.pub_key->encrypt(pt1_v[i]);
    ct2_v[i] = key.pub_key->encrypt(pt2_v[i]);
    ct_sum_v[i] = ct1_v[i] + ct2_v[i];
    dt_sum_v[i] = key.priv_key->decrypt(ct_sum_v[i]);
  }

  // verify result
  for (std::size_t i = 0; i < v_size; i++) {
    for (std::size_t j = 0; j < BATCH_SIZE; j++) {
      std::vector<uint32_t> v = dt_sum_v[i].getElementVec(j);
      uint64_t sum = v[0];
      if (v.size() > 1) sum = ((uint64_t)v[1] << 32) | v[0];

      uint64_t exp_sum = (uint64_t)exp_value1[j] + (uint64_t)exp_value2[j];

      EXPECT_EQ(sum, exp_sum);
    }
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest_OMP, CtPlusPtTest_OMP) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;
  std::size_t v_size = (num_values + BATCH_SIZE - 1) % BATCH_SIZE;
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  std::vector<uint32_t> exp_value1(BATCH_SIZE), exp_value2(BATCH_SIZE);
  for (int i = 0; i < v_size; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = dist(rng);
  }

  std::vector<ipcl::PlainText> pt1_v(v_size), pt2_v(v_size), dt_sum_v(v_size);
  std::vector<ipcl::CipherText> ct1_v(v_size), ct2_v(v_size), ct_sum_v(v_size);

#ifdef IPCL_BENCHMARK_USE_OMP
#pragma omp parallel for
#endif  // IPCL_BENCHMARK_USE_OMP
  for (std::size_t i = 0; i < v_size; i++) {
    pt1_v[i] = ipcl::PlainText(exp_value1);
    pt2_v[i] = ipcl::PlainText(exp_value2);
    ct1_v[i] = key.pub_key->encrypt(pt1_v[i]);
    ct_sum_v[i] = ct1_v[i] + pt2_v[i];
    dt_sum_v[i] = key.priv_key->decrypt(ct_sum_v[i]);
  }

  // verify result
  for (std::size_t i = 0; i < v_size; i++) {
    for (std::size_t j = 0; j < BATCH_SIZE; j++) {
      std::vector<uint32_t> v = dt_sum_v[i].getElementVec(j);
      uint64_t sum = v[0];
      if (v.size() > 1) sum = ((uint64_t)v[1] << 32) | v[0];

      uint64_t exp_sum = (uint64_t)exp_value1[j] + (uint64_t)exp_value2[j];

      EXPECT_EQ(sum, exp_sum);
    }
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest_OMP, CtMultiplyPtTest_OMP) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;
  std::size_t v_size = (num_values + BATCH_SIZE - 1) % BATCH_SIZE;
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  std::vector<uint32_t> exp_value1(BATCH_SIZE), exp_value2(BATCH_SIZE);
  for (int i = 0; i < v_size; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = dist(rng);
  }

  std::vector<ipcl::PlainText> pt1_v(v_size), pt2_v(v_size),
      dt_product_v(v_size);
  std::vector<ipcl::CipherText> ct1_v(v_size), ct2_v(v_size),
      ct_product_v(v_size);

#ifdef IPCL_BENCHMARK_USE_OMP
#pragma omp parallel for
#endif  // IPCL_BENCHMARK_USE_OMP
  for (std::size_t i = 0; i < v_size; i++) {
    pt1_v[i] = ipcl::PlainText(exp_value1);
    pt2_v[i] = ipcl::PlainText(exp_value2);
    ct1_v[i] = key.pub_key->encrypt(pt1_v[i]);
    ct_product_v[i] = ct1_v[i] * pt2_v[i];
    dt_product_v[i] = key.priv_key->decrypt(ct_product_v[i]);
  }

  // verify result
  for (std::size_t i = 0; i < v_size; i++) {
    for (std::size_t j = 0; j < BATCH_SIZE; j++) {
      std::vector<uint32_t> v = dt_product_v[i].getElementVec(j);
      uint64_t product = v[0];
      if (v.size() > 1) product = ((uint64_t)v[1] << 32) | v[0];

      uint64_t exp_product = (uint64_t)exp_value1[j] * (uint64_t)exp_value2[j];

      EXPECT_EQ(product, exp_product);
    }
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest_OMP, CtMultiplyZeroPtTest_OMP) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;
  std::size_t v_size = (num_values + BATCH_SIZE - 1) % BATCH_SIZE;
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  std::vector<uint32_t> exp_value1(BATCH_SIZE), exp_value2(BATCH_SIZE);
  for (int i = 0; i < v_size; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = 0;
  }

  std::vector<ipcl::PlainText> pt1_v(v_size), pt2_v(v_size),
      dt_product_v(v_size);
  std::vector<ipcl::CipherText> ct1_v(v_size), ct2_v(v_size),
      ct_product_v(v_size);

#ifdef IPCL_BENCHMARK_USE_OMP
#pragma omp parallel for
#endif  // IPCL_BENCHMARK_USE_OMP
  for (std::size_t i = 0; i < v_size; i++) {
    pt1_v[i] = ipcl::PlainText(exp_value1);
    pt2_v[i] = ipcl::PlainText(exp_value2);
    ct1_v[i] = key.pub_key->encrypt(pt1_v[i]);
    ct_product_v[i] = ct1_v[i] * pt2_v[i];
    dt_product_v[i] = key.priv_key->decrypt(ct_product_v[i]);
  }

  // verify result
  for (std::size_t i = 0; i < v_size; i++) {
    for (std::size_t j = 0; j < BATCH_SIZE; j++) {
      std::vector<uint32_t> v = dt_product_v[i].getElementVec(j);
      uint64_t product = v[0];
      if (v.size() > 1) product = ((uint64_t)v[1] << 32) | v[0];

      uint64_t exp_product = (uint64_t)exp_value1[j] * (uint64_t)exp_value2[j];

      EXPECT_EQ(product, exp_product);
    }
  }

  delete key.pub_key;
  delete key.priv_key;
}

void AddSub_OMP(std::vector<ipcl::CipherText>& res,
                const std::vector<ipcl::CipherText>& ct1,
                const std::vector<ipcl::CipherText>& ct2,
                const ipcl::keyPair key) {
  int size = ct1.size();
  std::vector<BigNumber> sum_bn_v(size);

  for (int i = 0; i < size; i++) {
    ipcl::CipherText a(ct1[i]);
    ipcl::CipherText b(ct2[i]);
    ipcl::PlainText m1(2);

    a = a + b * m1;
    res[i] = a + b;
  }
}

TEST(OperationTest_OMP, AddSubTest_OMP) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;
  std::size_t v_size = (num_values + BATCH_SIZE - 1) % BATCH_SIZE;
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  std::vector<uint32_t> exp_value1(BATCH_SIZE), exp_value2(BATCH_SIZE);
  for (int i = 0; i < v_size; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = dist(rng);
  }

  std::vector<ipcl::PlainText> pt1_v(v_size), pt2_v(v_size), dt_sum_v(v_size);
  std::vector<ipcl::CipherText> ct1_v(v_size), ct2_v(v_size), ct_sum_v(v_size);

#ifdef IPCL_BENCHMARK_USE_OMP
#pragma omp parallel for
#endif  // IPCL_BENCHMARK_USE_OMP
  for (std::size_t i = 0; i < v_size; i++) {
    pt1_v[i] = ipcl::PlainText(exp_value1);
    pt2_v[i] = ipcl::PlainText(exp_value2);
    ct1_v[i] = key.pub_key->encrypt(pt1_v[i]);
    ct2_v[i] = key.pub_key->encrypt(pt2_v[i]);

    // ct - ct
    ipcl::CipherText a(ct1_v[i]);
    ipcl::CipherText b(ct2_v[i]);
    std::vector<uint32_t> m_v(BATCH_SIZE, 2);
    ipcl::PlainText m1(m_v);
    a = a + b * m1;
    ct_sum_v[i] = a + b;

    dt_sum_v[i] = key.priv_key->decrypt(ct_sum_v[i]);
  }

  // verify result
  for (std::size_t i = 0; i < v_size; i++) {
    for (std::size_t j = 0; j < BATCH_SIZE; j++) {
      std::vector<uint32_t> v = dt_sum_v[i].getElementVec(j);
      uint64_t sum = v[0];
      if (v.size() > 1) sum = ((uint64_t)v[1] << 32) | v[0];

      uint64_t exp_sum = (uint64_t)exp_value1[j] + (uint64_t)exp_value2[j] * 3;

      EXPECT_EQ(sum, exp_sum);
    }
  }

  delete key.pub_key;
  delete key.priv_key;
}
