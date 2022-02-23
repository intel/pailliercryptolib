// Copyright (C) 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/paillier_ops.hpp"

#include <algorithm>

// constructors
//
PaillierEncryptedNumber::PaillierEncryptedNumber(PaillierPublicKey* pub_key,
                                                 const BigNumber& bn)
    : b_isObfuscator(false),
      m_available(1),
      m_pubkey(pub_key),
      m_length(1),
      m_bn{bn}  // m_bn[0]
{}

PaillierEncryptedNumber::PaillierEncryptedNumber(PaillierPublicKey* pub_key,
                                                 const BigNumber bn[8],
                                                 size_t length)
    : b_isObfuscator(false),
      m_pubkey(pub_key),
      m_available(8),
      m_length(length),
      m_bn{bn[0], bn[1], bn[2], bn[3], bn[4], bn[5], bn[6], bn[7]} {}

PaillierEncryptedNumber::PaillierEncryptedNumber(PaillierPublicKey* pub_key,
                                                 const uint32_t scalar[8],
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
  if (m_pubkey->getN() != other.m_pubkey->getN()) {
    throw std::runtime_error("two different public keys detected!!");
  }

  PaillierEncryptedNumber a = *this;
  PaillierEncryptedNumber b = other;

  if (m_available == 1) {
    BigNumber sum = a.raw_add(a.m_bn[0], b.m_bn[0]);
    return PaillierEncryptedNumber(m_pubkey, sum);
  } else {
    BigNumber sum[8];
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
    const BigNumber other[8]) const {
  PaillierEncryptedNumber a = *this;

  BigNumber b[8], sum[8];
  a.m_pubkey->encrypt(b, other, false);
  for (int i = 0; i < 8; i++) sum[i] = a.raw_add(a.m_bn[i], b[i]);
  return PaillierEncryptedNumber(m_pubkey, sum);
}

// CT*PT PaillierEncryptedNumber store a plaintext integer, not an encrypted
// integer
PaillierEncryptedNumber PaillierEncryptedNumber::operator*(
    const PaillierEncryptedNumber& other) const {
  if (m_pubkey->getN() != other.m_pubkey->getN()) {
    throw std::runtime_error("two different public keys detected!!");
  }

  PaillierEncryptedNumber a = *this;
  PaillierEncryptedNumber b = other;

  if (m_available == 1) {
    BigNumber product = a.raw_mul(a.m_bn[0], b.m_bn[0]);
    return PaillierEncryptedNumber(m_pubkey, product);
  } else {
    BigNumber product[8];
    a.raw_mul(product, a.m_bn, b.m_bn);
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
                                           const BigNumber& b) {
  // Hold a copy of nsquare for multi-threaded
  BigNumber sq = m_pubkey->getNSQ();
  return a * b % sq;
}

void PaillierEncryptedNumber::raw_mul(BigNumber res[8], BigNumber a[8],
                                      BigNumber b[8]) {
  BigNumber sq[8];
  for (int i = 0; i < 8; i++) sq[i] = m_pubkey->getNSQ();
  m_pubkey->ippMultiBuffExp(res, a, b, sq);
}

BigNumber PaillierEncryptedNumber::raw_mul(const BigNumber& a,
                                           const BigNumber& b) {
  BigNumber sq = m_pubkey->getNSQ();
  return m_pubkey->ippMontExp(a, b, sq);
}

PaillierEncryptedNumber PaillierEncryptedNumber::rotate(int shift) {
  if (m_available == 1)
    throw std::invalid_argument("Cannot rotate single PaillierEncryptedNumber");
  if (shift > 8 || shift < -8)
    throw std::invalid_argument("Cannot shift more than 8 or -8");

  if (shift == 0 || shift == 8 || shift == -8)
    return PaillierEncryptedNumber(m_pubkey, m_bn);

  if (shift > 0)
    shift = 8 - shift;
  else
    shift = -shift;

  BigNumber new_bn[8];
  getArrayBN(new_bn);

  std::rotate(std::begin(new_bn), std::begin(new_bn) + shift, std::end(new_bn));
  return PaillierEncryptedNumber(m_pubkey, new_bn);
}
