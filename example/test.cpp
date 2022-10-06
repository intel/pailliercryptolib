// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <climits>
#include <iostream>
#include <ipcl/ipcl.hpp>
#include <random>
#include <vector>

int main() {
  const uint32_t num_values = 9;

  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<uint32_t> exp_value(num_values);
  ipcl::PlainText pt;
  ipcl::CipherText ct;
  ipcl::PlainText dt;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_values; i++) {
    exp_value[i] = dist(rng);
  }

  pt = ipcl::PlainText(exp_value);
  ct = key.pub_key->encrypt(pt);
  dt = key.priv_key->decrypt(ct);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = dt.getElementVec(i);
    bool chk = v[0] == exp_value[i];
    std::cout << (chk ? "pass" : "fail") << std::endl;
  }

  delete key.pub_key;
  delete key.priv_key;
}
