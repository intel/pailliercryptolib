// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/paillier_ops.hpp"
#include "ipcl/mod_exp.hpp"

#include <algorithm>

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
      m_available(IPCL_CRYPTO_MB_SIZE),
      m_length(length),
      m_bn{bn[0], bn[1], bn[2], bn[3], bn[4], bn[5], bn[6], bn[7]} {}

PaillierEncryptedNumber::PaillierEncryptedNumber(
    const PaillierPublicKey* pub_key, const std::vector<uint32_t>& scalar,
    size_t length)
    : b_isObfuscator(false),
      m_pubkey(pub_key),
      m_available(IPCL_CRYPTO_MB_SIZE),
      m_length(length),
      m_bn{scalar[0], scalar[1], scalar[2], scalar[3],
           scalar[4], scalar[5], scalar[6], scalar[7]} {}

// CT+CT
PaillierEncryptedNumber PaillierEncryptedNumber::operator+(
    const PaillierEncryptedNumber& other) const {
  ERROR_CHECK(m_pubkey->getN() == other.m_pubkey->getN(),
              "operator+: two different public keys detected!!");

  const auto& a = *this;
  const auto& b = other;

  if (m_available == 1) {
    BigNumber sum = a.raw_add(a.m_bn.front(), b.m_bn.front());
    return PaillierEncryptedNumber(m_pubkey, sum);
  }

  std::vector<BigNumber> sum(IPCL_CRYPTO_MB_SIZE);
  for (int i = 0; i < m_available; i++)
    sum[i] = a.raw_add(a.m_bn[i], b.m_bn[i]);
  return PaillierEncryptedNumber(m_pubkey, sum);
}

// CT+PT
PaillierEncryptedNumber PaillierEncryptedNumber::operator+(
    const BigNumber& other) const {
  const auto& a = *this;
  BigNumber b;
  a.m_pubkey->encrypt(b, other);

  BigNumber sum = a.raw_add(a.m_bn.front(), b);
  return PaillierEncryptedNumber(m_pubkey, sum);
}

// multi encrypted CT+PT
PaillierEncryptedNumber PaillierEncryptedNumber::operator+(
    const std::vector<BigNumber>& other) const {
  VEC_SIZE_CHECK(other);

  const auto& a = *this;

  std::vector<BigNumber> b(IPCL_CRYPTO_MB_SIZE);
  std::vector<BigNumber> sum(IPCL_CRYPTO_MB_SIZE);
  a.m_pubkey->encrypt(b, other, false);
  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++)
    sum[i] = a.raw_add(a.m_bn[i], b[i]);
  return PaillierEncryptedNumber(m_pubkey, sum);
}

// CT*PT PaillierEncryptedNumber store a plaintext integer, not an encrypted
// integer
PaillierEncryptedNumber PaillierEncryptedNumber::operator*(
    const PaillierEncryptedNumber& other) const {
  ERROR_CHECK(m_pubkey->getN() == other.m_pubkey->getN(),
              "operator*: two different public keys detected!!");

  const auto& a = *this;
  const auto& b = other;

  if (m_available == 1) {
    BigNumber product = a.raw_mul(a.m_bn.front(), b.m_bn.front());
    return PaillierEncryptedNumber(m_pubkey, product);
  }

  std::vector<BigNumber> product = a.raw_mul(a.m_bn, b.m_bn);
  return PaillierEncryptedNumber(m_pubkey, product);
}

// CT*PT
PaillierEncryptedNumber PaillierEncryptedNumber::operator*(
    const BigNumber& other) const {
  const auto& a = *this;

  BigNumber product = a.raw_mul(a.m_bn.front(), other);
  return PaillierEncryptedNumber(m_pubkey, product);
}

BigNumber PaillierEncryptedNumber::raw_add(const BigNumber& a,
                                           const BigNumber& b) const {
  // Hold a copy of nsquare for multi-threaded
  const BigNumber& sq = m_pubkey->getNSQ();
  return a * b % sq;
}

std::vector<BigNumber> PaillierEncryptedNumber::raw_mul(
    const std::vector<BigNumber>& a, const std::vector<BigNumber>& b) const {
  std::vector<BigNumber> sq(IPCL_CRYPTO_MB_SIZE, m_pubkey->getNSQ());
  return ipcl::ippModExp(a, b, sq);
}

BigNumber PaillierEncryptedNumber::raw_mul(const BigNumber& a,
                                           const BigNumber& b) const {
  const BigNumber& sq = m_pubkey->getNSQ();
  return ipcl::ippModExp(a, b, sq);
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

void PaillierEncryptedNumber::applyObfuscator() {
  b_isObfuscator = true;
  std::vector<BigNumber> obfuscator(IPCL_CRYPTO_MB_SIZE);
  m_pubkey->applyObfuscator(obfuscator);

  const BigNumber& sq = m_pubkey->getNSQ();
  for (int i = 0; i < m_available; i++)
    m_bn[i] = sq.ModMul(m_bn[i], obfuscator[i]);
}

}  // namespace ipcl
