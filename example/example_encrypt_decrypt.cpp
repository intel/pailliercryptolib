// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

/*
  Example of encryption and decryption
*/
#include <climits>
#include <iostream>
#include <random>
#include <vector>

#include "ipcl/ipcl.hpp"

int main() {
  ipcl::initializeContext("QAT");

  const uint32_t num_total = 20;

  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<uint32_t> exp_value(num_total);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_total; i++) {
    exp_value[i] = dist(rng);
  }

  ipcl::PlainText pt = ipcl::PlainText(exp_value);

  ipcl::setHybridMode(ipcl::HybridMode::OPTIMAL);

  ipcl::CipherText ct = key.pub_key->encrypt(pt);
  ipcl::PlainText dt = key.priv_key->decrypt(ct);

  ipcl::setHybridOff();

  // verify result
  bool verify = true;
  for (int i = 0; i < num_total; i++) {
    std::vector<uint32_t> v = dt.getElementVec(i);
    if (v[0] != exp_value[i]) {
      verify = false;
      break;
    }
  }
  std::cout << "Test pt == dec(enc(pt)) -- " << (verify ? "pass" : "fail")
            << std::endl;

  delete key.pub_key;
  delete key.priv_key;
  ipcl::terminateContext();
}
