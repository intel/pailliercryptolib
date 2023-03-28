// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "ipcl/ipcl.hpp"

TEST(SerialTest, PublicKeyTest) {
  const int key_bits = 2048;
  ipcl::KeyPair key = ipcl::generateKeypair(key_bits);
  ipcl::PublicKey exp_pk = key.pub_key;
  ipcl::PrivateKey sk = key.priv_key;
  std::vector<Ipp32u> vec(key_bits / 32, 0);
  BigNumber bn(vec.data(), key_bits / 32);
  ipcl::PublicKey ret_pk(bn, key_bits);

  std::ostringstream os;
  ipcl::serializer::serialize(os, exp_pk);
  std::istringstream is(os.str());
  ipcl::serializer::deserialize(is, ret_pk);

  ipcl::PlainText pt(123);
  ipcl::CipherText ct = ret_pk.encrypt(pt);
  ipcl::PlainText dt = sk.decrypt(ct);
  EXPECT_EQ(pt.getElement(0), dt.getElement(0));
}
