// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <climits>
#include <iostream>
#include <random>
#include <vector>

#ifdef IPCL_USE_OMP
#include <omp.h>
#endif  // IPCL_USE_OMP

#include "gtest/gtest.h"
#include "ipcl/ciphertext.hpp"
#include "ipcl/keygen.hpp"
#include "ipcl/plaintext.hpp"

constexpr int SELF_DEF_NUM_VALUES = 20;

void CtPlusCt(ipcl::CipherText& res, const ipcl::CipherText& ct1,
              const ipcl::CipherText& ct2, const ipcl::keyPair key) {
  int size = ct1.getSize();
  std::vector<BigNumber> sum_bn_v(size);

  for (int i = 0; i < size; i++) {
    ipcl::CipherText a(key.pub_key, ct1.getElement(i));
    ipcl::CipherText b(key.pub_key, ct2.getElement(i));
    ipcl::CipherText sum = a + b;
    sum_bn_v[i] = sum.getElement(0);
  }
  res = ipcl::CipherText(key.pub_key, sum_bn_v);
}

void CtPlusCtArray(ipcl::CipherText& res, const ipcl::CipherText& ct1,
                   const ipcl::CipherText& ct2, const ipcl::keyPair key) {
  res = ct1 + ct2;
}

void CtPlusPt(ipcl::CipherText& res, const ipcl::CipherText& ct1,
              const ipcl::PlainText& pt2, const ipcl::keyPair key) {
  int size = ct1.getSize();
  std::vector<BigNumber> sum_bn_v(size);

  for (int i = 0; i < size; i++) {
    ipcl::CipherText a(key.pub_key, ct1.getElement(i));
    ipcl::PlainText b(pt2.getElement(i));
    ipcl::CipherText sum = a + b;
    sum_bn_v[i] = sum.getElement(0);
  }
  res = ipcl::CipherText(key.pub_key, sum_bn_v);
}

void CtPlusPtArray(ipcl::CipherText& res, const ipcl::CipherText& ct1,
                   const ipcl::PlainText& pt2, const ipcl::keyPair key) {
  res = ct1 + pt2;
}

void CtMultiplyPt(ipcl::CipherText& res, const ipcl::CipherText& ct1,
                  const ipcl::PlainText& pt2, const ipcl::keyPair key) {
  int size = ct1.getSize();
  std::vector<BigNumber> product_bn_v(size);

  for (int i = 0; i < size; i++) {
    ipcl::CipherText a(key.pub_key, ct1.getElement(i));
    ipcl::PlainText b(pt2.getElement(i));
    ipcl::CipherText product = a * b;
    product_bn_v[i] = product.getElement(0);
  }
  res = ipcl::CipherText(key.pub_key, product_bn_v);
}

void CtMultiplyPtArray(ipcl::CipherText& res, const ipcl::CipherText& ct1,
                       const ipcl::PlainText& pt2, const ipcl::keyPair key) {
  res = ct1 * pt2;
}

void AddSub(ipcl::CipherText& res, const ipcl::CipherText& ct1,
            const ipcl::CipherText& ct2, const ipcl::keyPair key) {
  int size = ct1.getSize();
  std::vector<BigNumber> sum_bn_v(size);

  for (int i = 0; i < size; i++) {
    ipcl::CipherText a(key.pub_key, ct1.getElement(i));
    ipcl::CipherText b(key.pub_key, ct2.getElement(i));
    ipcl::PlainText m1(2);

    a = a + b * m1;
    ipcl::CipherText sum = a + b;
    sum_bn_v[i] = sum.getElement(0);
  }
  res = ipcl::CipherText(key.pub_key, sum_bn_v);
}

TEST(OperationTest, CtPlusCtTest) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<uint32_t> exp_value1(num_values), exp_value2(num_values);
  ipcl::PlainText pt1, pt2, dt_sum;
  ipcl::CipherText ct1, ct2, ct_sum;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_values; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = dist(rng);
  }
  pt1 = ipcl::PlainText(exp_value1);
  pt2 = ipcl::PlainText(exp_value2);

  ct1 = key.pub_key->encrypt(pt1);
  ct2 = key.pub_key->encrypt(pt2);

  CtPlusCt(ct_sum, ct1, ct2, key);

  dt_sum = key.priv_key->decrypt(ct_sum);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = dt_sum.getElementVec(i);
    uint64_t sum = v[0];
    if (v.size() > 1) sum = ((uint64_t)v[1] << 32) | v[0];

    uint64_t exp_sum = (uint64_t)exp_value1[i] + (uint64_t)exp_value2[i];

    EXPECT_EQ(sum, exp_sum);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusCtArrayTest) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<uint32_t> exp_value1(num_values), exp_value2(num_values);
  ipcl::PlainText pt1, pt2, dt_sum;
  ipcl::CipherText ct1, ct2, ct_sum;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_values; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = dist(rng);
  }
  pt1 = ipcl::PlainText(exp_value1);
  pt2 = ipcl::PlainText(exp_value2);

  ct1 = key.pub_key->encrypt(pt1);
  ct2 = key.pub_key->encrypt(pt2);

  CtPlusCtArray(ct_sum, ct1, ct2, key);

  dt_sum = key.priv_key->decrypt(ct_sum);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = dt_sum.getElementVec(i);
    uint64_t sum = v[0];
    if (v.size() > 1) sum = ((uint64_t)v[1] << 32) | v[0];

    uint64_t exp_sum = (uint64_t)exp_value1[i] + (uint64_t)exp_value2[i];

    EXPECT_EQ(sum, exp_sum);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusPtTest) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<uint32_t> exp_value1(num_values), exp_value2(num_values);
  ipcl::PlainText pt1, pt2, dt_sum;
  ipcl::CipherText ct1, ct2, ct_sum;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_values; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = dist(rng);
  }
  pt1 = ipcl::PlainText(exp_value1);
  pt2 = ipcl::PlainText(exp_value2);

  ct1 = key.pub_key->encrypt(pt1);

  CtPlusPt(ct_sum, ct1, pt2, key);

  dt_sum = key.priv_key->decrypt(ct_sum);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = dt_sum.getElementVec(i);
    uint64_t sum = v[0];
    if (v.size() > 1) sum = ((uint64_t)v[1] << 32) | v[0];

    uint64_t exp_sum = (uint64_t)exp_value1[i] + (uint64_t)exp_value2[i];

    EXPECT_EQ(sum, exp_sum);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusPtArrayTest) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<uint32_t> exp_value1(num_values), exp_value2(num_values);
  ipcl::PlainText pt1, pt2, dt_sum;
  ipcl::CipherText ct1, ct2, ct_sum;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_values; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = dist(rng);
  }
  pt1 = ipcl::PlainText(exp_value1);
  pt2 = ipcl::PlainText(exp_value2);

  ct1 = key.pub_key->encrypt(pt1);

  CtPlusPtArray(ct_sum, ct1, pt2, key);

  dt_sum = key.priv_key->decrypt(ct_sum);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = dt_sum.getElementVec(i);
    uint64_t sum = v[0];
    if (v.size() > 1) sum = ((uint64_t)v[1] << 32) | v[0];

    uint64_t exp_sum = (uint64_t)exp_value1[i] + (uint64_t)exp_value2[i];

    EXPECT_EQ(sum, exp_sum);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyPtTest) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<uint32_t> exp_value1(num_values), exp_value2(num_values);
  ipcl::PlainText pt1, pt2, dt_product;
  ipcl::CipherText ct1, ct2, ct_product;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_values; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = dist(rng);
  }
  pt1 = ipcl::PlainText(exp_value1);
  pt2 = ipcl::PlainText(exp_value2);

  ct1 = key.pub_key->encrypt(pt1);

  CtMultiplyPt(ct_product, ct1, pt2, key);

  dt_product = key.priv_key->decrypt(ct_product);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = dt_product.getElementVec(i);
    uint64_t product = v[0];
    if (v.size() > 1) product = ((uint64_t)v[1] << 32) | v[0];

    uint64_t exp_product = (uint64_t)exp_value1[i] * (uint64_t)exp_value2[i];

    EXPECT_EQ(product, exp_product);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyZeroPtTest) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<uint32_t> exp_value1(num_values), exp_value2(num_values);
  ipcl::PlainText pt1, pt2, dt_product;
  ipcl::CipherText ct1, ct2, ct_product;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_values; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = 0;
  }

  pt1 = ipcl::PlainText(exp_value1);
  pt2 = ipcl::PlainText(exp_value2);

  ct1 = key.pub_key->encrypt(pt1);

  CtMultiplyPt(ct_product, ct1, pt2, key);

  dt_product = key.priv_key->decrypt(ct_product);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = dt_product.getElementVec(i);
    uint64_t product = v[0];
    if (v.size() > 1) product = ((uint64_t)v[1] << 32) | v[0];

    uint64_t exp_product = (uint64_t)exp_value1[i] * (uint64_t)exp_value2[i];

    EXPECT_EQ(product, exp_product);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyPtArrayTest) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<uint32_t> exp_value1(num_values), exp_value2(num_values);
  ipcl::PlainText pt1, pt2, dt_product;
  ipcl::CipherText ct1, ct2, ct_product;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_values; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = dist(rng);
  }
  pt1 = ipcl::PlainText(exp_value1);
  pt2 = ipcl::PlainText(exp_value2);

  ct1 = key.pub_key->encrypt(pt1);

  CtMultiplyPtArray(ct_product, ct1, pt2, key);

  dt_product = key.priv_key->decrypt(ct_product);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = dt_product.getElementVec(i);
    uint64_t product = v[0];
    if (v.size() > 1) product = ((uint64_t)v[1] << 32) | v[0];

    uint64_t exp_product = (uint64_t)exp_value1[i] * (uint64_t)exp_value2[i];

    EXPECT_EQ(product, exp_product);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, AddSubTest) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<uint32_t> exp_value1(num_values), exp_value2(num_values);
  ipcl::PlainText pt1, pt2, dt_sum;
  ipcl::CipherText ct1, ct2, ct_sum;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_values; i++) {
    exp_value1[i] = dist(rng);
    exp_value2[i] = dist(rng);
  }
  pt1 = ipcl::PlainText(exp_value1);
  pt2 = ipcl::PlainText(exp_value2);

  ct1 = key.pub_key->encrypt(pt1);
  ct2 = key.pub_key->encrypt(pt2);

  AddSub(ct_sum, ct1, ct2, key);

  dt_sum = key.priv_key->decrypt(ct_sum);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = dt_sum.getElementVec(i);
    uint64_t sum = v[0];
    if (v.size() > 1) sum = ((uint64_t)v[1] << 32) | v[0];

    uint64_t exp_sum = (uint64_t)exp_value1[i] + (uint64_t)exp_value2[i] * 3;

    EXPECT_EQ(sum, exp_sum);
  }

  delete key.pub_key;
  delete key.priv_key;
}

#ifdef IPCL_USE_OMP
void CtPlusCt_OMP(int num_threads, std::vector<ipcl::CipherText>& res_v,
                  const std::vector<ipcl::CipherText>& ct1_v,
                  const std::vector<ipcl::CipherText>& ct2_v,
                  const ipcl::keyPair key) {
  std::vector<std::vector<BigNumber>> sum_bn_v(num_threads);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    int size = ct1_v[i].getSize();
    sum_bn_v[i] = std::vector<BigNumber>(size);
    for (int j = 0; j < size; j++) {
      ipcl::CipherText a(key.pub_key, ct1_v[i].getElement(j));
      ipcl::CipherText b(key.pub_key, ct2_v[i].getElement(j));
      ipcl::CipherText sum = a + b;
      sum_bn_v[i][j] = sum.getElement(0);
    }
    res_v[i] = ipcl::CipherText(key.pub_key, sum_bn_v[i]);
  }
}

void CtPlusPt_OMP(int num_threads, std::vector<ipcl::CipherText>& res_v,
                  const std::vector<ipcl::CipherText>& ct1_v,
                  const std::vector<ipcl::PlainText>& pt2_v,
                  const ipcl::keyPair key) {
  std::vector<std::vector<BigNumber>> sum_bn_v(num_threads);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    int size = ct1_v[i].getSize();
    sum_bn_v[i] = std::vector<BigNumber>(size);
    for (int j = 0; j < size; j++) {
      ipcl::CipherText a(key.pub_key, ct1_v[i].getElement(j));
      ipcl::PlainText b(pt2_v[i].getElement(j));
      ipcl::CipherText sum = a + b;
      sum_bn_v[i][j] = sum.getElement(0);
    }
    res_v[i] = ipcl::CipherText(key.pub_key, sum_bn_v[i]);
  }
}

void CtPlusPtArray_OMP(int num_threads, std::vector<ipcl::CipherText>& res_v,
                       const std::vector<ipcl::CipherText>& ct1_v,
                       const std::vector<ipcl::PlainText>& pt2_v,
                       const ipcl::keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    res_v[i] = ct1_v[i] + pt2_v[i];
  }
}

void CtMultiplyPtArray_OMP(int num_threads,
                           std::vector<ipcl::CipherText>& res_v,
                           const std::vector<ipcl::CipherText>& ct1_v,
                           const std::vector<ipcl::PlainText>& pt2_v,
                           const ipcl::keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    res_v[i] = ct1_v[i] * pt2_v[i];
  }
}

void CtMultiplyPt_OMP(int num_threads, std::vector<ipcl::CipherText>& res_v,
                      const std::vector<ipcl::CipherText>& ct1_v,
                      const std::vector<ipcl::PlainText>& pt2_v,
                      const ipcl::keyPair key) {
  std::vector<std::vector<BigNumber>> product_bn_v(num_threads);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    int size = ct1_v[i].getSize();
    product_bn_v[i] = std::vector<BigNumber>(size);
    for (int j = 0; j < size; j++) {
      ipcl::CipherText a(key.pub_key, ct1_v[i].getElement(j));
      ipcl::PlainText b(pt2_v[i].getElement(j));
      ipcl::CipherText product = a * b;
      product_bn_v[i][j] = product.getElement(0);
    }
    res_v[i] = ipcl::CipherText(key.pub_key, product_bn_v[i]);
  }
}

TEST(OperationTest, CtPlusCtTest_OMP) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<std::vector<uint32_t>> exp_value1_vv(
      num_threads, std::vector<uint32_t>(num_values)),
      exp_value2_vv(num_threads, std::vector<uint32_t>(num_values));
  std::vector<ipcl::PlainText> pt1_v(num_threads), pt2_v(num_threads),
      dt_sum_v(num_threads);
  std::vector<ipcl::CipherText> ct1_v(num_threads), ct2_v(num_threads),
      ct_sum_v(num_threads);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
    // for each threads, generated different rand testing value
    for (int j = 0; j < num_values; j++) {
      exp_value1_vv[i][j] = dist(rng);
      exp_value2_vv[i][j] = dist(rng);
    }
    pt1_v[i] = ipcl::PlainText(exp_value1_vv[i]);
    pt2_v[i] = ipcl::PlainText(exp_value2_vv[i]);
  }

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    ct1_v[i] = key.pub_key->encrypt(pt1_v[i]);
    ct2_v[i] = key.pub_key->encrypt(pt2_v[i]);
  }

  CtPlusCt_OMP(num_threads, ct_sum_v, ct1_v, ct2_v, key);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    dt_sum_v[i] = key.priv_key->decrypt(ct_sum_v[i]);
  }

  // check output for all threads
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < num_values; j++) {
      std::vector<uint32_t> v = dt_sum_v[i].getElementVec(j);
      uint64_t sum = v[0];
      if (v.size() > 1) sum = ((uint64_t)v[1] << 32) | v[0];
      uint64_t exp_sum =
          (uint64_t)exp_value1_vv[i][j] + (uint64_t)exp_value2_vv[i][j];

      EXPECT_EQ(sum, exp_sum);
    }
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusPtTest_OMP) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<std::vector<uint32_t>> exp_value1_vv(
      num_threads, std::vector<uint32_t>(num_values)),
      exp_value2_vv(num_threads, std::vector<uint32_t>(num_values));
  std::vector<ipcl::PlainText> pt1_v(num_threads), pt2_v(num_threads),
      dt_sum_v(num_threads);
  std::vector<ipcl::CipherText> ct1_v(num_threads), ct2_v(num_threads),
      ct_sum_v(num_threads);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
    // for each threads, generated different rand testing value
    for (int j = 0; j < num_values; j++) {
      exp_value1_vv[i][j] = dist(rng);
      exp_value2_vv[i][j] = dist(rng);
    }
    pt1_v[i] = ipcl::PlainText(exp_value1_vv[i]);
    pt2_v[i] = ipcl::PlainText(exp_value2_vv[i]);
  }

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    ct1_v[i] = key.pub_key->encrypt(pt1_v[i]);
  }

  CtPlusPt_OMP(num_threads, ct_sum_v, ct1_v, pt2_v, key);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    dt_sum_v[i] = key.priv_key->decrypt(ct_sum_v[i]);
  }

  // check output for all threads
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < num_values; j++) {
      std::vector<uint32_t> v = dt_sum_v[i].getElementVec(j);
      uint64_t sum = v[0];
      if (v.size() > 1) sum = ((uint64_t)v[1] << 32) | v[0];
      uint64_t exp_sum =
          (uint64_t)exp_value1_vv[i][j] + (uint64_t)exp_value2_vv[i][j];

      EXPECT_EQ(sum, exp_sum);
    }
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusPtArrayTest_OMP) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<std::vector<uint32_t>> exp_value1_vv(
      num_threads, std::vector<uint32_t>(num_values)),
      exp_value2_vv(num_threads, std::vector<uint32_t>(num_values));
  std::vector<ipcl::PlainText> pt1_v(num_threads), pt2_v(num_threads),
      dt_sum_v(num_threads);
  std::vector<ipcl::CipherText> ct1_v(num_threads), ct2_v(num_threads),
      ct_sum_v(num_threads);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
    // for each threads, generated different rand testing value
    for (int j = 0; j < num_values; j++) {
      exp_value1_vv[i][j] = dist(rng);
      exp_value2_vv[i][j] = dist(rng);
    }
    pt1_v[i] = ipcl::PlainText(exp_value1_vv[i]);
    pt2_v[i] = ipcl::PlainText(exp_value2_vv[i]);
  }

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    ct1_v[i] = key.pub_key->encrypt(pt1_v[i]);
  }

  CtPlusPtArray_OMP(num_threads, ct_sum_v, ct1_v, pt2_v, key);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    dt_sum_v[i] = key.priv_key->decrypt(ct_sum_v[i]);
  }

  // check output for all threads
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < num_values; j++) {
      std::vector<uint32_t> v = dt_sum_v[i].getElementVec(j);
      uint64_t sum = v[0];
      if (v.size() > 1) sum = ((uint64_t)v[1] << 32) | v[0];
      uint64_t exp_sum =
          (uint64_t)exp_value1_vv[i][j] + (uint64_t)exp_value2_vv[i][j];

      EXPECT_EQ(sum, exp_sum);
    }
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyPtArrayTest_OMP) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<std::vector<uint32_t>> exp_value1_vv(
      num_threads, std::vector<uint32_t>(num_values)),
      exp_value2_vv(num_threads, std::vector<uint32_t>(num_values));
  std::vector<ipcl::PlainText> pt1_v(num_threads), pt2_v(num_threads),
      dt_product_v(num_threads);
  std::vector<ipcl::CipherText> ct1_v(num_threads), ct2_v(num_threads),
      ct_product_v(num_threads);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
    // for each threads, generated different rand testing value
    for (int j = 0; j < num_values; j++) {
      exp_value1_vv[i][j] = dist(rng);
      exp_value2_vv[i][j] = dist(rng);
    }
    pt1_v[i] = ipcl::PlainText(exp_value1_vv[i]);
    pt2_v[i] = ipcl::PlainText(exp_value2_vv[i]);
  }

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    ct1_v[i] = key.pub_key->encrypt(pt1_v[i]);
  }

  CtMultiplyPtArray_OMP(num_threads, ct_product_v, ct1_v, pt2_v, key);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    dt_product_v[i] = key.priv_key->decrypt(ct_product_v[i]);
  }

  // check output for all threads
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < num_values; j++) {
      std::vector<uint32_t> v = dt_product_v[i].getElementVec(j);
      uint64_t product = v[0];
      if (v.size() > 1) product = ((uint64_t)v[1] << 32) | v[0];
      uint64_t exp_product =
          (uint64_t)exp_value1_vv[i][j] * (uint64_t)exp_value2_vv[i][j];

      EXPECT_EQ(product, exp_product);
    }
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyPtTest_OMP) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<std::vector<uint32_t>> exp_value1_vv(
      num_threads, std::vector<uint32_t>(num_values)),
      exp_value2_vv(num_threads, std::vector<uint32_t>(num_values));
  std::vector<ipcl::PlainText> pt1_v(num_threads), pt2_v(num_threads),
      dt_product_v(num_threads);
  std::vector<ipcl::CipherText> ct1_v(num_threads), ct2_v(num_threads),
      ct_product_v(num_threads);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
    // for each threads, generated different rand testing value
    for (int j = 0; j < num_values; j++) {
      exp_value1_vv[i][j] = dist(rng);
      exp_value2_vv[i][j] = dist(rng);
    }
    pt1_v[i] = ipcl::PlainText(exp_value1_vv[i]);
    pt2_v[i] = ipcl::PlainText(exp_value2_vv[i]);
  }

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    ct1_v[i] = key.pub_key->encrypt(pt1_v[i]);
  }

  CtMultiplyPt_OMP(num_threads, ct_product_v, ct1_v, pt2_v, key);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    dt_product_v[i] = key.priv_key->decrypt(ct_product_v[i]);
  }

  // check output for all threads
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < num_values; j++) {
      std::vector<uint32_t> v = dt_product_v[i].getElementVec(j);
      uint64_t product = v[0];
      if (v.size() > 1) product = ((uint64_t)v[1] << 32) | v[0];
      uint64_t exp_product =
          (uint64_t)exp_value1_vv[i][j] * (uint64_t)exp_value2_vv[i][j];

      EXPECT_EQ(product, exp_product);
    }
  }

  delete key.pub_key;
  delete key.priv_key;
}

#endif  // IPCL_USE_OMP
