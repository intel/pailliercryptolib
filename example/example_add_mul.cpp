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
  std::cout << std::endl;
  std::cout << "==============================================" << std::endl;
  std::cout << "Example: Addition and Multiplication with IPCL" << std::endl;
  std::cout << "==============================================" << std::endl;

  ipcl::initializeContext("QAT");

  const uint32_t num_total = 20;

  ipcl::KeyPair key = ipcl::generateKeypair(2048, true);

  std::vector<uint32_t> x(num_total);
  std::vector<uint32_t> y(num_total);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0,
                                                                UINT_MAX >> 16);

  for (int i = 0; i < num_total; i++) {
    x[i] = dist(rng);
    y[i] = dist(rng);
  }

  ipcl::PlainText pt_x = ipcl::PlainText(x);
  ipcl::PlainText pt_y = ipcl::PlainText(y);

  ipcl::setHybridMode(ipcl::HybridMode::OPTIMAL);

  ipcl::CipherText ct_x = key.pub_key.encrypt(pt_x);
  ipcl::CipherText ct_y = key.pub_key.encrypt(pt_y);

  // Perform enc(x) + enc(y)
  std::cout << "--- IPCL CipherText + CipherText ---" << std::endl;
  ipcl::CipherText ct_add_ctx_cty = ct_x + ct_y;
  ipcl::PlainText dt_add_ctx_cty = key.priv_key.decrypt(ct_add_ctx_cty);

  // verify result
  bool verify = true;
  for (int i = 0; i < num_total; i++) {
    std::vector<uint32_t> v = dt_add_ctx_cty.getElementVec(i);
    if (v[0] != (x[i] + y[i])) {
      verify = false;
      break;
    }
  }
  std::cout << "Test (x + y) == dec(enc(x) + enc(y)) -- "
            << (verify ? "pass" : "fail") << std::endl
            << std::endl;

  // Perform enc(x) + y
  std::cout << "--- IPCL CipherText + PlainText ---" << std::endl;
  ipcl::CipherText ct_add_ctx_pty = ct_x + pt_y;
  ipcl::PlainText dt_add_ctx_pty = key.priv_key.decrypt(ct_add_ctx_pty);

  // verify result
  verify = true;
  for (int i = 0; i < num_total; i++) {
    std::vector<uint32_t> v = dt_add_ctx_pty.getElementVec(i);
    if (v[0] != (x[i] + y[i])) {
      verify = false;
      break;
    }
  }
  std::cout << "Test (x + y) == dec(enc(x) + y) -- "
            << (verify ? "pass" : "fail") << std::endl
            << std::endl;

  // Perform enc(x) * y
  std::cout << "--- IPCL CipherText * PlainText ---" << std::endl;
  ipcl::CipherText ct_mul_ctx_pty = ct_x * pt_y;
  ipcl::PlainText dt_mul_ctx_pty = key.priv_key.decrypt(ct_mul_ctx_pty);

  // verify result
  verify = true;
  for (int i = 0; i < num_total; i++) {
    std::vector<uint32_t> v = dt_mul_ctx_pty.getElementVec(i);
    if (v[0] != (x[i] * y[i])) {
      verify = false;
      break;
    }
  }
  std::cout << "Test (x * y) == dec(enc(x) * y) -- "
            << (verify ? "pass" : "fail") << std::endl;

  ipcl::setHybridOff();

  ipcl::terminateContext();
  std::cout << "Complete!" << std::endl;
}
