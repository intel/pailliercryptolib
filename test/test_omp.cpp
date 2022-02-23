// Copyright (C) 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>
#include <omp.h>

#include <climits>
#include <random>
#include <vector>

#include "ipcl/key_pair.hpp"
#include "ipcl/paillier_ops.hpp"

const int numRuns = 10;
double t1, t2;
double time_ms;

#define BENCHMARK(target, x)                                \
  for (int i = 0; i < 2; ++i) x t1 = omp_get_wtime();       \
  for (int i = 0; i < numRuns; ++i) x t2 = omp_get_wtime(); \
                                                            \
  time_ms = static_cast<double>(t2 - t1);                   \
  std::cout << target << " " << time_ms * 1000.f / numRuns << "ms" << std::endl;

void Encryption(int num_threads, vector<BigNumber*> v_ct,
                vector<BigNumber*> v_ptbn, keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    key.pub_key->encrypt(v_ct[i], v_ptbn[i]);
  }
}

void Decryption(int num_threads, vector<BigNumber*> v_dt,
                vector<BigNumber*> v_ct, keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    key.priv_key->decrypt(v_dt[i], v_ct[i]);
  }
}

void CtPlusCt(int num_threads, vector<BigNumber*> v_sum,
              vector<BigNumber*> v_ct1, vector<BigNumber*> v_ct2, keyPair key) {
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

void CtPlusPt(int num_threads, vector<BigNumber*> v_sum,
              vector<BigNumber*> v_ct1, vector<uint32_t*> v_pt2, keyPair key) {
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

void CtPlusPtArray(int num_threads, vector<BigNumber*> v_sum,
                   vector<BigNumber*> v_ct1, vector<uint32_t*> v_pt2,
                   keyPair key) {
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

void CtMultiplyPt(int num_threads, vector<BigNumber*> v_product,
                  vector<BigNumber*> v_ct1, vector<uint32_t*> v_pt2,
                  keyPair key) {
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

void CtMultiplyPtArray(int num_threads, vector<BigNumber*> v_product,
                       vector<BigNumber*> v_ct1, vector<uint32_t*> v_pt2,
                       keyPair key) {
#pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    PaillierEncryptedNumber a(key.pub_key, v_ct1[i]);
    PaillierEncryptedNumber b(key.pub_key, v_pt2[i]);
    PaillierEncryptedNumber product = a * b;
    product.getArrayBN(v_product[i]);
  }
}

TEST(OperationTest, CtPlusCtTest) {
  keyPair key = generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();
  // std::cout << "available threads: " << num_threads << std::endl;

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

  BENCHMARK("OMP Paillier CT + CT",
            CtPlusCt(num_threads, v_sum, v_ct1, v_ct2, key););

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

TEST(OperationTest, CtPlusPtTest) {
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

  BENCHMARK("OMP Paillier CT + PT",
            CtPlusPt(num_threads, v_sum, v_ct1, v_pt2, key););

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

TEST(OperationTest, CtPlusPtArrayTest) {
  keyPair key = generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();
  std::cout << "available threads: " << num_threads << std::endl;

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

  BENCHMARK("OMP array Paillier CT + PT",
            CtPlusPtArray(num_threads, v_sum, v_ct1, v_pt2, key););

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

TEST(OperationTest, test_CtMultiplyPt) {
  keyPair key = generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();
  // std::cout << "available threads: " << num_threads << std::endl;

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

  BENCHMARK("OMP Paillier CT * PT",
            CtMultiplyPt(num_threads, v_product, v_ct1, v_pt2, key););

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

TEST(OperationTest, test_CtMultiplyPtArray) {
  keyPair key = generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();
  // std::cout << "available threads: " << num_threads << std::endl;

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

  BENCHMARK("OMP Paillier multi-buffered CT * PT",
            CtMultiplyPtArray(num_threads, v_product, v_ct1, v_pt2, key););

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

TEST(CryptoTest, CryptoTest) {
  // use one keypair to do several encryption/decryption
  keyPair key = generateKeypair(2048);

  size_t num_threads = omp_get_max_threads();
  // std::cout << "available threads: " << num_threads << std::endl;

  std::vector<BigNumber*> v_ct(num_threads);
  std::vector<BigNumber*> v_dt(num_threads);
  std::vector<uint32_t*> v_pt(num_threads);
  std::vector<BigNumber*> v_ptbn(num_threads);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_threads; i++) {
    v_ct[i] = new BigNumber[8];
    v_dt[i] = new BigNumber[8];
    v_pt[i] = new uint32_t[8];
    v_ptbn[i] = new BigNumber[8];

    // for each threads, generated different rand testing value
    for (int j = 0; j < 8; j++) {
      v_pt[i][j] = dist(rng);
      v_ptbn[i][j] = v_pt[i][j];
    }
  }

  BENCHMARK("OMP Paillier Encryption",
            Encryption(num_threads, v_ct, v_ptbn, key););
  BENCHMARK("OMP Paillier Decryption",
            Decryption(num_threads, v_dt, v_ct, key););

  // check output for all threads
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < 8; j++) {
      std::vector<Ipp32u> v;
      v_dt[i][j].num2vec(v);
      EXPECT_EQ(v[0], v_pt[i][j]);
    }
  }

  for (int i = 0; i < num_threads; i++) {
    delete[] v_ct[i];
    delete[] v_dt[i];
    delete[] v_pt[i];
    delete[] v_ptbn[i];
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
