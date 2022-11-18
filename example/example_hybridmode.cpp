// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

/*
  Example of encryption and decryption
*/
#include <chrono>  // NOLINT [build/c++11]
#include <climits>
#include <iostream>
#include <random>
#include <vector>

#include "ipcl/ipcl.hpp"

typedef std::chrono::high_resolution_clock::time_point tVar;
#define tNow() std::chrono::high_resolution_clock::now()
#define tStart(t) t = tNow()
#define tEnd(t) \
  std::chrono::duration_cast<std::chrono::milliseconds>(tNow() - t).count()

int main() {
  std::cout << std::endl;
  std::cout << "===================================" << std::endl;
  std::cout << "Example: Hybrid Mode usage with QAT" << std::endl;
  std::cout << "===================================" << std::endl;

  ipcl::initializeContext("QAT");
  tVar t;
  double elapsed(0.);

  const uint32_t num_total = 64;

  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<BigNumber> exp_value(num_total);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);
  BigNumber bigNum =
      "0xff03b1a74827c746db83d2eaff00067622f545b62584321256e62b01509f10962f9c5c"
      "8fd0b7f5184a9ce8e81f439df47dda14563dd55a221799d2aa57ed2713271678a5a0b8b4"
      "0a84ad13d5b6e6599e6467c670109cf1f45ccfed8f75ea3b814548ab294626fe4d14ff76"
      "4dd8b091f11a0943a2dd2b983b0df02f4c4d00b413acaabc1dc57faa9fd6a4274c4d5887"
      "65a1d3311c22e57d8101431b07eb3ddcb05d77d9a742ac2322fe6a063bd1e05acb13b0fe"
      "91c70115c2b1eee1155e072527011a5f849de7072a1ce8e6b71db525fbcda7a89aaed46d"
      "27aca5eaeaf35a26270a4a833c5cda681ffd49baa0f610bad100cdf47cc86e5034e2a0b2"
      "179e04ec7";

  for (int i = 0; i < num_total; i++) {
    exp_value[i] = bigNum - dist(rng);
  }

  ipcl::PlainText pt = ipcl::PlainText(exp_value);

  // Encrypt/Decrypt - IPP-Crypto only mode
  ipcl::setHybridMode(ipcl::HybridMode::IPP);
  tStart(t);
  ipcl::CipherText ct = key.pub_key->encrypt(pt);
  elapsed = tEnd(t);
  std::cout << " Encrypt - HybridMode::IPP     = " << elapsed << "ms"
            << std::endl;
  tStart(t);
  ipcl::PlainText dt = key.priv_key->decrypt(ct);
  elapsed = tEnd(t);
  std::cout << " Decrypt - HybridMode::IPP     = " << elapsed << "ms"
            << std::endl
            << std::endl;

  // Encrypt/Decrypt - QAT only mode
  ipcl::setHybridMode(ipcl::HybridMode::QAT);
  tStart(t);
  ct = key.pub_key->encrypt(pt);
  elapsed = tEnd(t);
  std::cout << " Encrypt - HybridMode::QAT     = " << elapsed << "ms"
            << std::endl;
  tStart(t);
  dt = key.priv_key->decrypt(ct);
  elapsed = tEnd(t);
  std::cout << " Decrypt - HybridMode::QAT     = " << elapsed << "ms"
            << std::endl
            << std::endl;

  // Encrypt/Decrypt - OPTIMAL mode
  ipcl::setHybridMode(ipcl::HybridMode::OPTIMAL);
  tStart(t);
  ct = key.pub_key->encrypt(pt);
  elapsed = tEnd(t);
  std::cout << " Encrypt - HybridMode::OPTIMAL = " << elapsed << "ms"
            << std::endl;
  tStart(t);
  dt = key.priv_key->decrypt(ct);
  elapsed = tEnd(t);
  std::cout << " Decrypt - HybridMode::OPTIMAL = " << elapsed << "ms"
            << std::endl
            << std::endl;

  delete key.pub_key;
  delete key.priv_key;
  ipcl::terminateContext();
  std::cout << "Complete!" << std::endl << std::endl;
}
