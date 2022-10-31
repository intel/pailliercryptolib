// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <climits>
#include <ipcl/ipcl.hpp>
#include <random>
#include <vector>

int main() {
  ipcl::initializeContext("QAT");

  const uint32_t num_total = 19;
  const uint32_t num_qat = 10;

  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<uint32_t> exp_value(num_total);
  ipcl::PlainText pt;
  ipcl::CipherText ct;
  ipcl::PlainText dt;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_total; i++) {
    exp_value[i] = dist(rng);
  }

  pt = ipcl::PlainText(exp_value);

  ipcl::setHybridModExp(num_total, num_qat);

  ct = key.pub_key->encrypt(pt);
  dt = key.priv_key->decrypt(ct);

  ipcl::setHybridModExpOff();

  for (int i = 0; i < num_total; i++) {
    std::vector<uint32_t> v = dt.getElementVec(i);
    bool chk = v[0] == exp_value[i];
    std::cout << (chk ? "pass" : "fail") << std::endl;
  }

  delete key.pub_key;
  delete key.priv_key;
  ipcl::terminateContext();
}
