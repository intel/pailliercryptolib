// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <climits>
#include <iostream>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "ipcl/ipcl.hpp"

constexpr int SELF_DEF_NUM_VALUES = 7;

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

void PtPlusCt(ipcl::CipherText& res, const ipcl::PlainText& pt2,
              const ipcl::CipherText& ct1, const ipcl::keyPair key) {
  int size = ct1.getSize();
  std::vector<BigNumber> sum_bn_v(size);

  for (int i = 0; i < size; i++) {
    ipcl::CipherText a(key.pub_key, ct1.getElement(i));
    ipcl::PlainText b(pt2.getElement(i));
    ipcl::CipherText sum = b + a;
    sum_bn_v[i] = sum.getElement(0);
  }
  res = ipcl::CipherText(key.pub_key, sum_bn_v);
}

void PtPlusCtArray(ipcl::CipherText& res, const ipcl::PlainText& pt2,
                   const ipcl::CipherText& ct1, const ipcl::keyPair key) {
  res = pt2 + ct1;
}

void PtMultiplyCt(ipcl::CipherText& res, const ipcl::PlainText& pt2,
                  const ipcl::CipherText& ct1, const ipcl::keyPair key) {
  int size = ct1.getSize();
  std::vector<BigNumber> product_bn_v(size);

  for (int i = 0; i < size; i++) {
    ipcl::CipherText a(key.pub_key, ct1.getElement(i));
    ipcl::PlainText b(pt2.getElement(i));
    ipcl::CipherText product = b * a;
    product_bn_v[i] = product.getElement(0);
  }
  res = ipcl::CipherText(key.pub_key, product_bn_v);
}

void PtMultiplyCtArray(ipcl::CipherText& res, const ipcl::PlainText& pt2,
                       const ipcl::CipherText& ct1, const ipcl::keyPair key) {
  res = pt2 * ct1;
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

TEST(OperationTest, PtPlusCtTest) {
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

  PtPlusCt(ct_sum, pt2, ct1, key);

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

TEST(OperationTest, PtPlusCtArrayTest) {
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

  PtPlusCtArray(ct_sum, pt2, ct1, key);

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

TEST(OperationTest, PtMultiplyCtTest) {
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

  PtMultiplyCt(ct_product, pt2, ct1, key);

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

TEST(OperationTest, PtMultiplyCtArrayTest) {
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

  PtMultiplyCtArray(ct_product, pt2, ct1, key);

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
