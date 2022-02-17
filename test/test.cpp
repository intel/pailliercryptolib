// Copyright (C) 2021-2022 Intel Corporation

#include <climits>
#include <iostream>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "ipcl/key_pair.hpp"
#include "ipcl/paillier_ops.hpp"

const int numRuns = 10;
clock_t t1, t2;
double time_ms;

#define BENCHMARK(target, x)                               \
  for (int i = 0; i < 2; ++i) x t1 = clock();              \
  for (int i = 0; i < numRuns; ++i) x t2 = clock();        \
                                                           \
  time_ms = static_cast<double>(t2 - t1) / CLOCKS_PER_SEC; \
  std::cout << target << " " << time_ms * 1000 / numRuns << "ms" << std::endl;

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

  BENCHMARK("Paillier CT + CT", CtPlusCt(res, ct1, ct2, key););

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    vector<Ipp32u> v;
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

  BENCHMARK("Paillier array CT + CT", CtPlusCtArray(res, ct1, ct2, key););

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    vector<Ipp32u> v;
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

  BENCHMARK("Paillier CT + PT", CtPlusPt(res, ct1, pt2, key););

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    vector<Ipp32u> v;
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

  BENCHMARK("Paillier array CT + PT", CtPlusPtArray(res, ct1, ptbn2, key););

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    vector<Ipp32u> v;
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

  BENCHMARK("Paillier CT * PT", CtMultiplyPt(res, ct1, pt2, key););

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    vector<Ipp32u> v;
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

  BENCHMARK("Paillier multi-buffered CT * PT",
            CtMultiplyPtArray(res, ct1, pt2, key););

  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    vector<Ipp32u> v;
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

  BENCHMARK("Paillier addsub", AddSub(res, ct1, ct2, key););
  key.priv_key->decrypt(dt, res);

  for (int i = 0; i < 8; i++) {
    vector<Ipp32u> v;
    dt[i].num2vec(v);

    uint64_t v_pp = (uint64_t)pt1[i] + (uint64_t)pt2[i] * 3;
    uint64_t v_dp = v[0];
    if (v.size() > 1) v_dp = ((uint64_t)v[1] << 32) | v[0];

    EXPECT_EQ(v_dp, v_pp);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(CryptoTest, CryptoTest) {
  keyPair key = generateKeypair(2048);

  BigNumber ct[8];
  BigNumber dt[8];

  uint32_t pt[8];
  BigNumber ptbn[8];
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < 8; i++) {
    pt[i] = dist(rng);
    ptbn[i] = pt[i];
  }

  BENCHMARK("Paillier Encryption", key.pub_key->encrypt(ct, ptbn););
  BENCHMARK("Paillier Decryption", key.priv_key->decrypt(dt, ct););

  for (int i = 0; i < 8; i++) {
    vector<Ipp32u> v;
    dt[i].num2vec(v);
    EXPECT_EQ(v[0], pt[i]);
  }

  delete key.pub_key;
  delete key.priv_key;
}

int main(int argc, char** argv) {
  // Use system clock for seed
  srand(time(nullptr));
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
