// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <climits>
#include <random>

#include "gtest/gtest.h"
#include "ipcl/ipcl.hpp"

constexpr int SELF_DEF_NUM_VALUES = 14;
constexpr int SELF_DEF_KEY_SIZE = 2048;

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

TEST(SerialTest, PlaintextTest) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  std::vector<uint32_t> exp_value(num_values);
  std::for_each(exp_value.begin(), exp_value.end(),
                [&](uint32_t& n) { n = dist(rng); });

  ipcl::PlainText pt = ipcl::PlainText(exp_value);
  std::ostringstream os;
  ipcl::serializer::serialize(os, pt);

  ipcl::PlainText pt_after;
  std::istringstream is(os.str());
  ipcl::serializer::deserialize(is, pt_after);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = pt.getElementVec(i);
    std::vector<uint32_t> v_after = pt_after.getElementVec(i);

    EXPECT_EQ(v[0], v_after[0]);
  }
}

TEST(SerialTest, CipherText) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;
  ipcl::KeyPair keys = ipcl::generateKeypair(SELF_DEF_KEY_SIZE);

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  std::vector<uint32_t> exp_value(num_values);
  std::for_each(exp_value.begin(), exp_value.end(),
                [&](uint32_t& n) { n = dist(rng); });

  ipcl::CipherText ct = ipcl::CipherText(keys.pub_key, exp_value);
  std::ostringstream os;
  ipcl::serializer::serialize(os, ct);

  ipcl::PlainText ct_after;
  std::istringstream is(os.str());
  ipcl::serializer::deserialize(is, ct_after);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = ct.getElementVec(i);
    std::vector<uint32_t> v_after = ct_after.getElementVec(i);

    EXPECT_EQ(v[0], v_after[0]);
  }
}
