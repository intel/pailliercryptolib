// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <climits>
#include <iostream>
#include <random>
#include <vector>

#ifdef IPCL_UNITTEST_OMP
#include <omp.h>
#endif

#include "gtest/gtest.h"
#include "ipcl/key_pair.hpp"
#include "ipcl/paillier_ops.hpp"

void CtPlusCt(BigNumber res[8], BigNumber ct1[8], BigNumber ct2[8],
              keyPair key) {
  for (int i = 0; i < 8; i++) {
    PaillierEncryptedNumber a(key.pub_key, ct1[i]);
    PaillierEncryptedNumber b(key.pub_key, ct2[i]);
    PaillierEncryptedNumber sum = a + b;
    res[i] = sum.getBN();
  }
}

void CtPlusCtArray(BigNumber res[8], BigNumber ct1[8], BigNumber ct2[8],
                   keyPair key) {
  PaillierEncryptedNumber a(key.pub_key, ct1);
  PaillierEncryptedNumber b(key.pub_key, ct2);
  PaillierEncryptedNumber sum = a + b;
  sum.getArrayBN(res);
}

void CtPlusPt(BigNumber res[8], BigNumber ct1[8], uint32_t pt2[8],
              keyPair key) {
  for (int i = 0; i < 8; i++) {
    PaillierEncryptedNumber a(key.pub_key, ct1[i]);
    BigNumber b = pt2[i];
    PaillierEncryptedNumber sum = a + b;
    res[i] = sum.getBN();
  }
}

void CtPlusPtArray(BigNumber res[8], BigNumber ct1[8], BigNumber ptbn2[8],
                   keyPair key) {
  BigNumber stmp[8];
  PaillierEncryptedNumber a(key.pub_key, ct1);
  PaillierEncryptedNumber sum = a + ptbn2;
  sum.getArrayBN(res);
}

void CtMultiplyPt(BigNumber res[8], BigNumber ct1[8], uint32_t pt2[8],
                  keyPair key) {
  for (int i = 0; i < 8; i++) {
    PaillierEncryptedNumber a(key.pub_key, ct1[i]);
    PaillierEncryptedNumber b(key.pub_key, pt2[i]);
    PaillierEncryptedNumber sum = a * b;
    res[i] = sum.getBN();
  }
}

void CtMultiplyPtArray(BigNumber res[8], BigNumber ct1[8], uint32_t pt2[8],
                       keyPair key) {
  PaillierEncryptedNumber a(key.pub_key, ct1);
  PaillierEncryptedNumber b(key.pub_key, pt2);
  PaillierEncryptedNumber sum = a * b;
  sum.getArrayBN(res);
}

void AddSub(BigNumber res[8], BigNumber ct1[8], BigNumber ct2[8], keyPair key) {
  for (int i = 0; i < 8; i++) {
    PaillierEncryptedNumber a(key.pub_key, ct1[i]);
    PaillierEncryptedNumber b(key.pub_key, ct2[i]);
    BigNumber m1(2);
    a = a + b * m1;
    PaillierEncryptedNumber sum = a + b;
    res[i] = sum.getBN();
  }
}

TEST(OperationTest, CtPlusCtTest) {
  keyPair key = generateKeypair(2048);

  BigNumber ct1[8], ct2[8];
  BigNumber dt[8], res[8];

  uint32_t pt1[8], pt2[8];
  BigNumber ptbn1[8], ptbn2[8];
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < 8; i++) {
    pt1[i] = dist(rng);
    pt2[i] = dist(rng);
    ptbn1[i] = pt1[i];
    ptbn2[i] = pt2[i];
  }

  key.pub_key->encrypt(ct1, ptbn1);
  key.pub_key->encrypt(ct2, ptbn2);

  CtPlusCt(res, ct1, ct2, key);

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    std::vector<Ipp32u> v;
    dt[i].num2vec(v);

    uint64_t v_pp = (uint64_t)pt1[i] + (uint64_t)pt2[i];
    uint64_t v_dp = v[0];
    if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

    EXPECT_EQ(v_dp, v_pp);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusCtArrayTest) {
  keyPair key = generateKeypair(2048);

  BigNumber ct1[8], ct2[8];
  BigNumber dt[8], res[8];

  uint32_t pt1[8], pt2[8];
  BigNumber ptbn1[8], ptbn2[8];
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < 8; i++) {
    pt1[i] = dist(rng);
    pt2[i] = dist(rng);
    ptbn1[i] = pt1[i];
    ptbn2[i] = pt2[i];
  }

  key.pub_key->encrypt(ct1, ptbn1);
  key.pub_key->encrypt(ct2, ptbn2);

  CtPlusCtArray(res, ct1, ct2, key);

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    std::vector<Ipp32u> v;
    dt[i].num2vec(v);

    uint64_t v_pp = (uint64_t)pt1[i] + (uint64_t)pt2[i];
    uint64_t v_dp = v[0];
    if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

    EXPECT_EQ(v_dp, v_pp);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusPtTest) {
  keyPair key = generateKeypair(2048);

  BigNumber ct1[8], ct2[8];
  BigNumber dt[8], res[8];

  uint32_t pt1[8], pt2[8];
  BigNumber ptbn1[8];
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < 8; i++) {
    pt1[i] = dist(rng);
    pt2[i] = dist(rng);
    ptbn1[i] = pt1[i];
  }

  key.pub_key->encrypt(ct1, ptbn1);

  CtPlusPt(res, ct1, pt2, key);

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    std::vector<Ipp32u> v;
    dt[i].num2vec(v);

    uint64_t v_pp = (uint64_t)pt1[i] + (uint64_t)pt2[i];
    uint64_t v_dp = v[0];
    if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

    EXPECT_EQ(v_dp, v_pp);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusPtArrayTest) {
  keyPair key = generateKeypair(2048);

  BigNumber ct1[8], ct2[8];
  BigNumber dt[8], res[8];

  uint32_t pt1[8], pt2[8];
  BigNumber ptbn1[8], ptbn2[8];
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < 8; i++) {
    pt1[i] = dist(rng);
    pt2[i] = dist(rng);
    ptbn1[i] = pt1[i];
    ptbn2[i] = pt2[i];
  }

  key.pub_key->encrypt(ct1, ptbn1);

  CtPlusPtArray(res, ct1, ptbn2, key);

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    std::vector<Ipp32u> v;
    dt[i].num2vec(v);

    uint64_t v_pp = (uint64_t)pt1[i] + (uint64_t)pt2[i];
    uint64_t v_dp = v[0];
    if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

    EXPECT_EQ(v_dp, v_pp);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyPtTest) {
  keyPair key = generateKeypair(2048);

  BigNumber ct1[8], ct2[8];
  BigNumber dt[8], res[8];

  uint32_t pt1[8], pt2[8];
  BigNumber ptbn1[8];
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < 8; i++) {
    pt1[i] = dist(rng);
    pt2[i] = dist(rng);
    ptbn1[i] = pt1[i];
  }

  key.pub_key->encrypt(ct1, ptbn1);

  CtMultiplyPt(res, ct1, pt2, key);

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    std::vector<Ipp32u> v;
    dt[i].num2vec(v);

    uint64_t v_pp = (uint64_t)pt1[i] * (uint64_t)pt2[i];
    uint64_t v_dp = v[0];
    if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

    EXPECT_EQ(v_dp, v_pp);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyZeroPtTest) {
  keyPair key = generateKeypair(2048);

  BigNumber ct1[8], ct2[8];
  BigNumber dt[8], res[8];

  uint32_t pt1[8], pt2[8];
  BigNumber ptbn1[8];
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < 8; i++) {
    pt1[i] = dist(rng);
    pt2[i] = 0;
    ptbn1[i] = pt1[i];
  }

  key.pub_key->encrypt(ct1, ptbn1);

  CtMultiplyPt(res, ct1, pt2, key);

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    std::vector<Ipp32u> v;
    dt[i].num2vec(v);

    uint64_t v_pp = (uint64_t)pt1[i] * (uint64_t)pt2[i];
    uint64_t v_dp = v[0];
    if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

    EXPECT_EQ(v_dp, v_pp);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyPtArrayTest) {
  keyPair key = generateKeypair(2048);

  BigNumber ct1[8];
  BigNumber dt[8], res[8];

  uint32_t pt1[8], pt2[8];
  BigNumber ptbn1[8], ptbn2[8];
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < 8; i++) {
    pt1[i] = dist(rng);
    pt2[i] = dist(rng);
    ptbn1[i] = pt1[i];
    ptbn2[i] = pt2[i];
  }

  key.pub_key->encrypt(ct1, ptbn1);

  CtMultiplyPtArray(res, ct1, pt2, key);

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    std::vector<Ipp32u> v;
    dt[i].num2vec(v);

    uint64_t v_pp = (uint64_t)pt1[i] * (uint64_t)pt2[i];
    uint64_t v_dp = v[0];
    if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

    EXPECT_EQ(v_dp, v_pp);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, AddSubTest) {
  keyPair key = generateKeypair(2048);

  BigNumber ct1[8], ct2[8];
  BigNumber dt[8], res[8];

  uint32_t pt1[8], pt2[8];
  BigNumber ptbn1[8], ptbn2[8];
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < 8; i++) {
    pt1[i] = dist(rng);
    pt2[i] = dist(rng);
    ptbn1[i] = pt1[i];
    ptbn2[i] = pt2[i];
  }

  key.pub_key->encrypt(ct1, ptbn1);
  key.pub_key->encrypt(ct2, ptbn2);

  AddSub(res, ct1, ct2, key);
  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    std::vector<Ipp32u> v;
    dt[i].num2vec(v);

    uint64_t v_pp = (uint64_t)pt1[i] + (uint64_t)pt2[i] * 3;
    uint64_t v_dp = v[0];
    if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

    EXPECT_EQ(v_dp, v_pp);
  }

  delete key.pub_key;
  delete key.priv_key;
}

#ifdef IPCL_UNITTEST_OMP
void CtPlusCt_OMP(int num_threads, std::vector<BigNumber*> v_sum,
                  std::vector<BigNumber*> v_ct1, std::vector<BigNumber*> v_ct2,
                  keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      PaillierEncryptedNumber a(key.pub_key, v_ct1[i][j]);
      PaillierEncryptedNumber b(key.pub_key, v_ct2[i][j]);
      PaillierEncryptedNumber sum = a + b;
      v_sum[i][j] = sum.getBN();
    }
  }
}

void CtPlusPt_OMP(int num_threads, std::vector<BigNumber*> v_sum,
                  std::vector<BigNumber*> v_ct1, std::vector<uint32_t*> v_pt2,
                  keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      BigNumber b = v_pt2[i][j];
      PaillierEncryptedNumber a(key.pub_key, v_ct1[i][j]);
      PaillierEncryptedNumber sum = a + b;
      v_sum[i][j] = sum.getBN();
    }
  }
}

void CtPlusPtArray_OMP(int num_threads, std::vector<BigNumber*> v_sum,
                       std::vector<BigNumber*> v_ct1,
                       std::vector<uint32_t*> v_pt2, keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    PaillierEncryptedNumber a(key.pub_key, v_ct1[i]);
    BigNumber b[8];
    for (int j = 0; j < 8; j++) {
      b[j] = v_pt2[i][j];
    }
    PaillierEncryptedNumber sum = a + b;
    sum.getArrayBN(v_sum[i]);
  }
}

void CtMultiplyPt_OMP(int num_threads, std::vector<BigNumber*> v_product,
                      std::vector<BigNumber*> v_ct1,
                      std::vector<uint32_t*> v_pt2, keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      PaillierEncryptedNumber a(key.pub_key, v_ct1[i][j]);
      BigNumber b = v_pt2[i][j];
      PaillierEncryptedNumber product = a * b;
      v_product[i][j] = product.getBN();
    }
  }
}

void CtMultiplyPtArray_OMP(int num_threads, std::vector<BigNumber*> v_product,
                           std::vector<BigNumber*> v_ct1,
                           std::vector<uint32_t*> v_pt2, keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    PaillierEncryptedNumber a(key.pub_key, v_ct1[i]);
    PaillierEncryptedNumber b(key.pub_key, v_pt2[i]);
    PaillierEncryptedNumber product = a * b;
    product.getArrayBN(v_product[i]);
  }
}

TEST(OperationTest, CtPlusCtTest_OMP) {
  keyPair key = generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<BigNumber*> v_ct1(num_threads);
  std::vector<BigNumber*> v_ct2(num_threads);
  std::vector<BigNumber*> v_dt(num_threads);
  std::vector<BigNumber*> v_sum(num_threads);
  std::vector<uint32_t*> v_pt1(num_threads);
  std::vector<uint32_t*> v_pt2(num_threads);
  std::vector<BigNumber*> v_ptbn1(num_threads);
  std::vector<BigNumber*> v_ptbn2(num_threads);
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
    v_ct1[i] = new BigNumber[8];
    v_ct2[i] = new BigNumber[8];
    v_dt[i] = new BigNumber[8];
    v_sum[i] = new BigNumber[8];
    v_pt1[i] = new uint32_t[8];
    v_pt2[i] = new uint32_t[8];
    v_ptbn1[i] = new BigNumber[8];
    v_ptbn2[i] = new BigNumber[8];

    // for each threads, generated different rand testing value
    for (int j = 0; j < 8; j++) {
      v_pt1[i][j] = dist(rng);
      v_pt2[i][j] = dist(rng);
      v_ptbn1[i][j] = v_pt1[i][j];
      v_ptbn2[i][j] = v_pt2[i][j];
    }
  }

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    key.pub_key->encrypt(v_ct1[i], v_ptbn1[i]);
    key.pub_key->encrypt(v_ct2[i], v_ptbn2[i]);
  }

  CtPlusCt_OMP(num_threads, v_sum, v_ct1, v_ct2, key);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    key.priv_key->decrypt(v_dt[i], v_sum[i]);
  }

  // check output for all threads
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      std::vector<Ipp32u> v;
      v_dt[i][j].num2vec(v);

      uint64_t v_pp = (uint64_t)v_pt1[i][j] + (uint64_t)v_pt2[i][j];
      uint64_t v_dp = v[0];
      if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

      EXPECT_EQ(v_dp, v_pp);
    }
  }

  for (int i = 0; i < num_threads; i++) {
    delete[] v_ct1[i];
    delete[] v_ct2[i];
    delete[] v_dt[i];
    delete[] v_pt1[i];
    delete[] v_pt2[i];
    delete[] v_ptbn1[i];
    delete[] v_ptbn2[i];
    delete[] v_sum[i];
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusPtTest_OMP) {
  keyPair key = generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<BigNumber*> v_ct1(num_threads);
  std::vector<BigNumber*> v_dt(num_threads);
  std::vector<BigNumber*> v_sum(num_threads);
  std::vector<uint32_t*> v_pt1(num_threads);
  std::vector<uint32_t*> v_pt2(num_threads);
  std::vector<BigNumber*> v_ptbn1(num_threads);
  std::vector<BigNumber*> v_ptbn2(num_threads);
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
    v_ct1[i] = new BigNumber[8];
    v_dt[i] = new BigNumber[8];
    v_sum[i] = new BigNumber[8];
    v_pt1[i] = new uint32_t[8];
    v_pt2[i] = new uint32_t[8];
    v_ptbn1[i] = new BigNumber[8];
    v_ptbn2[i] = new BigNumber[8];

    // for each threads, generated different rand testing value
    for (int j = 0; j < 8; j++) {
      v_pt1[i][j] = dist(rng);
      v_pt2[i][j] = dist(rng);
      v_ptbn1[i][j] = v_pt1[i][j];
      v_ptbn2[i][j] = v_pt2[i][j];
    }
  }

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    key.pub_key->encrypt(v_ct1[i], v_ptbn1[i]);
  }

  CtPlusPt_OMP(num_threads, v_sum, v_ct1, v_pt2, key);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    key.priv_key->decrypt(v_dt[i], v_sum[i]);
  }

  // check output for all threads
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      std::vector<Ipp32u> v;
      v_dt[i][j].num2vec(v);

      uint64_t v_pp = (uint64_t)v_pt1[i][j] + (uint64_t)v_pt2[i][j];
      uint64_t v_dp = v[0];
      if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

      EXPECT_EQ(v_dp, v_pp);
    }
  }

  for (int i = 0; i < num_threads; i++) {
    delete[] v_ct1[i];
    delete[] v_dt[i];
    delete[] v_pt1[i];
    delete[] v_pt2[i];
    delete[] v_ptbn1[i];
    delete[] v_ptbn2[i];
    delete[] v_sum[i];
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusPtArrayTest_OMP) {
  keyPair key = generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<BigNumber*> v_ct1(num_threads);
  std::vector<BigNumber*> v_dt(num_threads);
  std::vector<BigNumber*> v_sum(num_threads);
  std::vector<uint32_t*> v_pt1(num_threads);
  std::vector<uint32_t*> v_pt2(num_threads);
  std::vector<BigNumber*> v_ptbn1(num_threads);
  std::vector<BigNumber*> v_ptbn2(num_threads);
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
    v_ct1[i] = new BigNumber[8];
    v_dt[i] = new BigNumber[8];
    v_sum[i] = new BigNumber[8];
    v_pt1[i] = new uint32_t[8];
    v_pt2[i] = new uint32_t[8];
    v_ptbn1[i] = new BigNumber[8];
    v_ptbn2[i] = new BigNumber[8];

    // for each threads, generated different rand testing value
    for (int j = 0; j < 8; j++) {
      v_pt1[i][j] = dist(rng);
      v_pt2[i][j] = dist(rng);
      v_ptbn1[i][j] = v_pt1[i][j];
      v_ptbn2[i][j] = v_pt2[i][j];
    }
  }

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    key.pub_key->encrypt(v_ct1[i], v_ptbn1[i]);
  }

  CtPlusPtArray_OMP(num_threads, v_sum, v_ct1, v_pt2, key);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    key.priv_key->decrypt(v_dt[i], v_sum[i]);
  }

  // check output for all threads
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      std::vector<Ipp32u> v;
      v_dt[i][j].num2vec(v);

      uint64_t v_pp = (uint64_t)v_pt1[i][j] + (uint64_t)v_pt2[i][j];
      uint64_t v_dp = v[0];
      if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

      EXPECT_EQ(v_dp, v_pp);
    }
  }

  for (int i = 0; i < num_threads; i++) {
    delete[] v_ct1[i];
    delete[] v_dt[i];
    delete[] v_pt1[i];
    delete[] v_pt2[i];
    delete[] v_ptbn1[i];
    delete[] v_ptbn2[i];
    delete[] v_sum[i];
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyPtTest_OMP) {
  keyPair key = generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<BigNumber*> v_ct1(num_threads);
  std::vector<BigNumber*> v_dt(num_threads);
  std::vector<BigNumber*> v_product(num_threads);
  std::vector<uint32_t*> v_pt1(num_threads);
  std::vector<uint32_t*> v_pt2(num_threads);
  std::vector<BigNumber*> v_ptbn1(num_threads);
  std::vector<BigNumber*> v_ptbn2(num_threads);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
    v_ct1[i] = new BigNumber[8];
    v_dt[i] = new BigNumber[8];
    v_product[i] = new BigNumber[8];
    v_pt1[i] = new uint32_t[8];
    v_pt2[i] = new uint32_t[8];
    v_ptbn1[i] = new BigNumber[8];
    v_ptbn2[i] = new BigNumber[8];

    // for each threads, generated different rand testing value
    for (int j = 0; j < 8; j++) {
      v_pt1[i][j] = dist(rng);
      v_pt2[i][j] = dist(rng);
      v_ptbn1[i][j] = v_pt1[i][j];
      v_ptbn2[i][j] = v_pt2[i][j];
    }
  }

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    key.pub_key->encrypt(v_ct1[i], v_ptbn1[i]);
  }

  CtMultiplyPt_OMP(num_threads, v_product, v_ct1, v_pt2, key);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    key.priv_key->decrypt(v_dt[i], v_product[i]);
  }

  // check output for all threads
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      std::vector<Ipp32u> v;
      v_dt[i][j].num2vec(v);

      uint64_t v_pp = (uint64_t)v_pt1[i][j] * (uint64_t)v_pt2[i][j];
      uint64_t v_dp = v[0];
      if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];
      EXPECT_EQ(v_dp, v_pp);
    }
  }

  for (int i = 0; i < num_threads; i++) {
    delete[] v_ct1[i];
    delete[] v_dt[i];
    delete[] v_pt1[i];
    delete[] v_pt2[i];
    delete[] v_ptbn1[i];
    delete[] v_ptbn2[i];
    delete[] v_product[i];
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyPtArrayTest_OMP) {
  keyPair key = generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<BigNumber*> v_ct1(num_threads);
  std::vector<BigNumber*> v_dt(num_threads);
  std::vector<BigNumber*> v_product(num_threads);
  std::vector<uint32_t*> v_pt1(num_threads);
  std::vector<uint32_t*> v_pt2(num_threads);
  std::vector<BigNumber*> v_ptbn1(num_threads);
  std::vector<BigNumber*> v_ptbn2(num_threads);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
    v_ct1[i] = new BigNumber[8];
    v_dt[i] = new BigNumber[8];
    v_product[i] = new BigNumber[8];
    v_pt1[i] = new uint32_t[8];
    v_pt2[i] = new uint32_t[8];
    v_ptbn1[i] = new BigNumber[8];
    v_ptbn2[i] = new BigNumber[8];

    // for each threads, generated different rand testing value

    for (int j = 0; j < 8; j++) {
      v_pt1[i][j] = dist(rng);
      v_pt2[i][j] = dist(rng);
      v_ptbn1[i][j] = v_pt1[i][j];
      v_ptbn2[i][j] = v_pt2[i][j];
    }
  }

  for (int i = 0; i < num_threads; i++) {
    key.pub_key->encrypt(v_ct1[i], v_ptbn1[i]);
  }

  CtMultiplyPtArray_OMP(num_threads, v_product, v_ct1, v_pt2, key);

#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    key.priv_key->decrypt(v_dt[i], v_product[i]);
  }

  // check output for all threads
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      std::vector<Ipp32u> v;
      v_dt[i][j].num2vec(v);

      uint64_t v_pp = (uint64_t)v_pt1[i][j] * (uint64_t)v_pt2[i][j];
      uint64_t v_dp = v[0];
      if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];
      EXPECT_EQ(v_dp, v_pp);
    }
  }

  for (int i = 0; i < num_threads; i++) {
    delete[] v_ct1[i];
    delete[] v_dt[i];
    delete[] v_pt1[i];
    delete[] v_pt2[i];
    delete[] v_ptbn1[i];
    delete[] v_ptbn2[i];
    delete[] v_product[i];
  }

  delete key.pub_key;
  delete key.priv_key;
}
#endif
