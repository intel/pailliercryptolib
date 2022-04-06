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
#include "ipcl/keygen.hpp"
#include "ipcl/ops.hpp"

void CtPlusCt(std::vector<BigNumber>& res, const std::vector<BigNumber>& ct1,
              const std::vector<BigNumber>& ct2, const ipcl::keyPair key) {
  for (int i = 0; i < 8; i++) {
    ipcl::EncryptedNumber a(key.pub_key, ct1[i]);
    ipcl::EncryptedNumber b(key.pub_key, ct2[i]);
    ipcl::EncryptedNumber sum = a + b;
    res[i] = sum.getBN();
  }
}

void CtPlusCtArray(std::vector<BigNumber>& res,
                   const std::vector<BigNumber>& ct1,
                   const std::vector<BigNumber>& ct2, const ipcl::keyPair key) {
  ipcl::EncryptedNumber a(key.pub_key, ct1);
  ipcl::EncryptedNumber b(key.pub_key, ct2);
  ipcl::EncryptedNumber sum = a + b;
  res = sum.getArrayBN();
}

void CtPlusPt(std::vector<BigNumber>& res, const std::vector<BigNumber>& ct1,
              const std::vector<uint32_t>& pt2, const ipcl::keyPair key) {
  for (int i = 0; i < 8; i++) {
    ipcl::EncryptedNumber a(key.pub_key, ct1[i]);
    BigNumber b = pt2[i];
    ipcl::EncryptedNumber sum = a + b;
    res[i] = sum.getBN();
  }
}

void CtPlusPtArray(std::vector<BigNumber>& res,
                   const std::vector<BigNumber>& ct1,
                   const std::vector<BigNumber>& ptbn2,
                   const ipcl::keyPair key) {
  ipcl::EncryptedNumber a(key.pub_key, ct1);
  ipcl::EncryptedNumber sum = a + ptbn2;
  res = sum.getArrayBN();
}

void CtMultiplyPt(std::vector<BigNumber>& res,
                  const std::vector<BigNumber>& ct1,
                  const std::vector<uint32_t>& pt2, const ipcl::keyPair key) {
  for (int i = 0; i < 8; i++) {
    ipcl::EncryptedNumber a(key.pub_key, ct1[i]);
    ipcl::EncryptedNumber b(key.pub_key, pt2[i]);
    ipcl::EncryptedNumber sum = a * b;
    res[i] = sum.getBN();
  }
}

void CtMultiplyPtArray(std::vector<BigNumber>& res,
                       const std::vector<BigNumber>& ct1,
                       const std::vector<uint32_t>& pt2,
                       const ipcl::keyPair key) {
  ipcl::EncryptedNumber a(key.pub_key, ct1);
  ipcl::EncryptedNumber b(key.pub_key, pt2);
  ipcl::EncryptedNumber sum = a * b;
  res = sum.getArrayBN();
}

void AddSub(std::vector<BigNumber>& res, const std::vector<BigNumber>& ct1,
            const std::vector<BigNumber>& ct2, const ipcl::keyPair key) {
  for (int i = 0; i < 8; i++) {
    ipcl::EncryptedNumber a(key.pub_key, ct1[i]);
    ipcl::EncryptedNumber b(key.pub_key, ct2[i]);
    BigNumber m1(2);
    a = a + b * m1;
    ipcl::EncryptedNumber sum = a + b;
    res[i] = sum.getBN();
  }
}

TEST(OperationTest, CtPlusCtTest) {
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<BigNumber> ct1(8), ct2(8);
  std::vector<BigNumber> dt(8), res(8);

  std::vector<uint32_t> pt1(8), pt2(8);
  std::vector<BigNumber> ptbn1(8), ptbn2(8);
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
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<BigNumber> ct1(8), ct2(8);
  std::vector<BigNumber> dt(8), res(8);

  std::vector<uint32_t> pt1(8), pt2(8);
  std::vector<BigNumber> ptbn1(8), ptbn2(8);
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
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<BigNumber> ct1(8), ct2(8);
  std::vector<BigNumber> dt(8), res(8);

  std::vector<uint32_t> pt1(8), pt2(8);
  std::vector<BigNumber> ptbn1(8);
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
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<BigNumber> ct1(8), ct2(8);
  std::vector<BigNumber> dt(8), res(8);

  std::vector<uint32_t> pt1(8), pt2(8);
  std::vector<BigNumber> ptbn1(8), ptbn2(8);
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
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<BigNumber> ct1(8), ct2(8);
  std::vector<BigNumber> dt(8), res(8);

  std::vector<uint32_t> pt1(8), pt2(8);
  std::vector<BigNumber> ptbn1(8);
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
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<BigNumber> ct1(8), ct2(8);
  std::vector<BigNumber> dt(8), res(8);

  std::vector<uint32_t> pt1(8), pt2(8);
  std::vector<BigNumber> ptbn1(8);
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
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<BigNumber> ct1(8);
  std::vector<BigNumber> dt(8), res(8);

  std::vector<uint32_t> pt1(8), pt2(8);
  std::vector<BigNumber> ptbn1(8), ptbn2(8);
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
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  std::vector<BigNumber> ct1(8), ct2(8);
  std::vector<BigNumber> dt(8), res(8);

  std::vector<uint32_t> pt1(8), pt2(8);
  std::vector<BigNumber> ptbn1(8), ptbn2(8);
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

#ifdef IPCL_USE_OMP
void CtPlusCt_OMP(int num_threads, std::vector<std::vector<BigNumber>>& v_sum,
                  const std::vector<std::vector<BigNumber>>& v_ct1,
                  const std::vector<std::vector<BigNumber>>& v_ct2,
                  const ipcl::keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      ipcl::EncryptedNumber a(key.pub_key, v_ct1[i][j]);
      ipcl::EncryptedNumber b(key.pub_key, v_ct2[i][j]);
      ipcl::EncryptedNumber sum = a + b;
      v_sum[i][j] = sum.getBN();
    }
  }
}

void CtPlusPt_OMP(int num_threads, std::vector<std::vector<BigNumber>>& v_sum,
                  const std::vector<std::vector<BigNumber>>& v_ct1,
                  const std::vector<std::vector<uint32_t>>& v_pt2,
                  const ipcl::keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      BigNumber b = v_pt2[i][j];
      ipcl::EncryptedNumber a(key.pub_key, v_ct1[i][j]);
      ipcl::EncryptedNumber sum = a + b;
      v_sum[i][j] = sum.getBN();
    }
  }
}

void CtPlusPtArray_OMP(int num_threads,
                       std::vector<std::vector<BigNumber>>& v_sum,
                       const std::vector<std::vector<BigNumber>>& v_ct1,
                       const std::vector<std::vector<uint32_t>>& v_pt2,
                       const ipcl::keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    ipcl::EncryptedNumber a(key.pub_key, v_ct1[i]);
    std::vector<BigNumber> b(8);
    for (int j = 0; j < 8; j++) {
      b[j] = v_pt2[i][j];
    }
    ipcl::EncryptedNumber sum = a + b;
    v_sum[i] = sum.getArrayBN();
  }
}

void CtMultiplyPt_OMP(int num_threads,
                      std::vector<std::vector<BigNumber>>& v_product,
                      const std::vector<std::vector<BigNumber>>& v_ct1,
                      const std::vector<std::vector<uint32_t>>& v_pt2,
                      const ipcl::keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      ipcl::EncryptedNumber a(key.pub_key, v_ct1[i][j]);
      BigNumber b = v_pt2[i][j];
      ipcl::EncryptedNumber product = a * b;
      v_product[i][j] = product.getBN();
    }
  }
}

void CtMultiplyPtArray_OMP(int num_threads,
                           std::vector<std::vector<BigNumber>>& v_product,
                           const std::vector<std::vector<BigNumber>>& v_ct1,
                           const std::vector<std::vector<uint32_t>>& v_pt2,
                           const ipcl::keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    ipcl::EncryptedNumber a(key.pub_key, v_ct1[i]);
    ipcl::EncryptedNumber b(key.pub_key, v_pt2[i]);
    ipcl::EncryptedNumber product = a * b;
    v_product[i] = product.getArrayBN();
  }
}

TEST(OperationTest, CtPlusCtTest_OMP) {
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<std::vector<BigNumber>> v_ct1(num_threads,
                                            std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_ct2(num_threads,
                                            std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_dt(num_threads,
                                           std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_sum(num_threads,
                                            std::vector<BigNumber>(8));
  std::vector<std::vector<uint32_t>> v_pt1(num_threads,
                                           std::vector<uint32_t>(8));
  std::vector<std::vector<uint32_t>> v_pt2(num_threads,
                                           std::vector<uint32_t>(8));
  std::vector<std::vector<BigNumber>> v_ptbn1(num_threads,
                                              std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_ptbn2(num_threads,
                                              std::vector<BigNumber>(8));
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
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

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusPtTest_OMP) {
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<std::vector<BigNumber>> v_ct1(num_threads,
                                            std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_dt(num_threads,
                                           std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_sum(num_threads,
                                            std::vector<BigNumber>(8));
  std::vector<std::vector<uint32_t>> v_pt1(num_threads,
                                           std::vector<uint32_t>(8));
  std::vector<std::vector<uint32_t>> v_pt2(num_threads,
                                           std::vector<uint32_t>(8));
  std::vector<std::vector<BigNumber>> v_ptbn1(num_threads,
                                              std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_ptbn2(num_threads,
                                              std::vector<BigNumber>(8));
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
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

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtPlusPtArrayTest_OMP) {
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<std::vector<BigNumber>> v_ct1(num_threads,
                                            std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_dt(num_threads,
                                           std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_sum(num_threads,
                                            std::vector<BigNumber>(8));
  std::vector<std::vector<uint32_t>> v_pt1(num_threads,
                                           std::vector<uint32_t>(8));
  std::vector<std::vector<uint32_t>> v_pt2(num_threads,
                                           std::vector<uint32_t>(8));
  std::vector<std::vector<BigNumber>> v_ptbn1(num_threads,
                                              std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_ptbn2(num_threads,
                                              std::vector<BigNumber>(8));
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
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

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyPtTest_OMP) {
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<std::vector<BigNumber>> v_ct1(num_threads,
                                            std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_dt(num_threads,
                                           std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_product(num_threads,
                                                std::vector<BigNumber>(8));
  std::vector<std::vector<uint32_t>> v_pt1(num_threads,
                                           std::vector<uint32_t>(8));
  std::vector<std::vector<uint32_t>> v_pt2(num_threads,
                                           std::vector<uint32_t>(8));
  std::vector<std::vector<BigNumber>> v_ptbn1(num_threads,
                                              std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_ptbn2(num_threads,
                                              std::vector<BigNumber>(8));

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
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

  delete key.pub_key;
  delete key.priv_key;
}

TEST(OperationTest, CtMultiplyPtArrayTest_OMP) {
  ipcl::keyPair key = ipcl::generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();

  std::vector<std::vector<BigNumber>> v_ct1(num_threads,
                                            std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_dt(num_threads,
                                           std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_product(num_threads,
                                                std::vector<BigNumber>(8));
  std::vector<std::vector<uint32_t>> v_pt1(num_threads,
                                           std::vector<uint32_t>(8));
  std::vector<std::vector<uint32_t>> v_pt2(num_threads,
                                           std::vector<uint32_t>(8));
  std::vector<std::vector<BigNumber>> v_ptbn1(num_threads,
                                              std::vector<BigNumber>(8));
  std::vector<std::vector<BigNumber>> v_ptbn2(num_threads,
                                              std::vector<BigNumber>(8));

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
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

  delete key.pub_key;
  delete key.priv_key;
}
#endif  // IPCL_USE_OMP
