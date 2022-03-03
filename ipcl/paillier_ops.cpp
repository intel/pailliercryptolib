// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/paillier_ops.hpp"

#include <algorithm>

#include "ipcl/util.hpp"

namespace ipcl {
// constructors
//
PaillierEncryptedNumber::PaillierEncryptedNumber(
    const PaillierPublicKey* pub_key, const BigNumber& bn)
    : b_isObfuscator(false),
      m_available(1),
      m_pubkey(pub_key),
      m_length(1),
      m_bn{bn} {}

PaillierEncryptedNumber::PaillierEncryptedNumber(
    const PaillierPublicKey* pub_key, const std::vector<BigNumber>& bn,
    size_t length)
    : b_isObfuscator(false),
      m_pubkey(pub_key),
      m_available(8),
      m_length(length),
      m_bn{bn[0], bn[1], bn[2], bn[3], bn[4], bn[5], bn[6], bn[7]} {}

PaillierEncryptedNumber::PaillierEncryptedNumber(
    const PaillierPublicKey* pub_key, const std::vector<uint32_t>& scalar,
    size_t length)
    : b_isObfuscator(false),
      m_pubkey(pub_key),
      m_available(8),
      m_length(length),
      m_bn{scalar[0], scalar[1], scalar[2], scalar[3],
           scalar[4], scalar[5], scalar[6], scalar[7]} {}

// CT+CT
PaillierEncryptedNumber PaillierEncryptedNumber::operator+(
    const PaillierEncryptedNumber& other) const {
  ERROR_CHECK(m_pubkey->getN() == other.m_pubkey->getN(),
              "operator+: two different public keys detected!!");

  PaillierEncryptedNumber a = *this;
  PaillierEncryptedNumber b = other;

  if (m_available == 1) {
    BigNumber sum = a.raw_add(a.m_bn[0], b.m_bn[0]);
    return PaillierEncryptedNumber(m_pubkey, sum);
  } else {
    std::vector<BigNumber> sum(8);
    for (int i = 0; i < m_available; i++)
      sum[i] = a.raw_add(a.m_bn[i], b.m_bn[i]);
    return PaillierEncryptedNumber(m_pubkey, sum);
  }
}

// CT+PT
PaillierEncryptedNumber PaillierEncryptedNumber::operator+(
    const BigNumber& other) const {
  PaillierEncryptedNumber a = *this;
  BigNumber b;
  a.m_pubkey->encrypt(b, other);

  BigNumber sum = a.raw_add(a.m_bn[0], b);
  return PaillierEncryptedNumber(m_pubkey, sum);
}

// multi encrypted CT+PT
PaillierEncryptedNumber PaillierEncryptedNumber::operator+(
    const std::vector<BigNumber>& other) const {
  VEC_SIZE_CHECK(other);

  PaillierEncryptedNumber a = *this;

  std::vector<BigNumber> b(8);
  std::vector<BigNumber> sum(8);
  a.m_pubkey->encrypt(b, other, false);
  for (int i = 0; i < 8; i++) sum[i] = a.raw_add(a.m_bn[i], b[i]);
  return PaillierEncryptedNumber(m_pubkey, sum);
}

// CT*PT PaillierEncryptedNumber store a plaintext integer, not an encrypted
// integer
PaillierEncryptedNumber PaillierEncryptedNumber::operator*(
    const PaillierEncryptedNumber& other) const {
  ERROR_CHECK(m_pubkey->getN() == other.m_pubkey->getN(),
              "operator*: two different public keys detected!!");

  PaillierEncryptedNumber a = *this;
  PaillierEncryptedNumber b = other;

  if (m_available == 1) {
    BigNumber product = a.raw_mul(a.m_bn[0], b.m_bn[0]);
    return PaillierEncryptedNumber(m_pubkey, product);
  } else {
    std::vector<BigNumber> product = a.raw_mul(a.m_bn, b.m_bn);
    return PaillierEncryptedNumber(m_pubkey, product);
  }
}

// CT*PT
PaillierEncryptedNumber PaillierEncryptedNumber::operator*(
    const BigNumber& other) const {
  PaillierEncryptedNumber a = *this;

  BigNumber b = other;
  BigNumber product = a.raw_mul(a.m_bn[0], b);
  return PaillierEncryptedNumber(m_pubkey, product);
}

BigNumber PaillierEncryptedNumber::raw_add(const BigNumber& a,
                                           const BigNumber& b) const {
  // Hold a copy of nsquare for multi-threaded
  BigNumber sq = m_pubkey->getNSQ();
  return a * b % sq;
}

std::vector<BigNumber> PaillierEncryptedNumber::raw_mul(
    const std::vector<BigNumber>& a, const std::vector<BigNumber>& b) const {
  std::vector<BigNumber> sq(8, m_pubkey->getNSQ());
  return m_pubkey->ippModExp(a, b, sq);
}

BigNumber PaillierEncryptedNumber::raw_mul(const BigNumber& a,
                                           const BigNumber& b) const {
  BigNumber sq = m_pubkey->getNSQ();
  return m_pubkey->ippModExp(a, b, sq);
}

PaillierEncryptedNumber PaillierEncryptedNumber::rotate(int shift) const {
  ERROR_CHECK(m_available != 1,
              "rotate: Cannot rotate single PaillierEncryptedNumber");
  ERROR_CHECK(shift >= -8 && shift <= 8,
              "rotate: Cannot shift more than 8 or -8");

  if (shift == 0 || shift == 8 || shift == -8)
    return PaillierEncryptedNumber(m_pubkey, m_bn);

  if (shift > 0)
    shift = 8 - shift;
  else
    shift = -shift;

  std::vector<BigNumber> new_bn = getArrayBN();

  std::rotate(std::begin(new_bn), std::begin(new_bn) + shift, std::end(new_bn));
  return PaillierEncryptedNumber(m_pubkey, new_bn);
}

}  // namespace ipcl
