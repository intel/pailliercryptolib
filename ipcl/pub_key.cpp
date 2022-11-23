// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/pub_key.hpp"

#include <algorithm>
#include <climits>
#include <cstring>
#include <random>

#include "crypto_mb/exp.h"
#include "ipcl/ciphertext.hpp"
#include "ipcl/mod_exp.hpp"
#include "ipcl/utils/util.hpp"

namespace ipcl {

PublicKey::PublicKey(const BigNumber& n, int bits, bool enableDJN_)
    : m_n(std::make_shared<BigNumber>(n)),
      m_g(std::make_shared<BigNumber>(*m_n + 1)),
      m_nsquare(std::make_shared<BigNumber>((*m_n) * (*m_n))),
      m_bits(bits),
      m_dwords(BITSIZE_DWORD(m_bits * 2)),
      m_enable_DJN(false),
      m_testv(false),
      m_hs(0),
      m_randbits(0) {
  if (enableDJN_) this->enableDJN();  // sets m_enable_DJN
  m_isInitialized = true;
}

void PublicKey::enableDJN() {
  BigNumber gcd;
  BigNumber rmod;
  do {
    int rand_bit = (*m_n).BitSize();
    BigNumber rand = getRandomBN(rand_bit + 128);
    rmod = rand % (*m_n);
    gcd = rand.gcd(*m_n);
  } while (gcd.compare(1));

  BigNumber rmod_sq = rmod * rmod;
  BigNumber rmod_neg = rmod_sq * -1;
  BigNumber h = rmod_neg % (*m_n);
  m_hs = modExp(h, *m_n, *m_nsquare);
  m_randbits = m_bits >> 1;  // bits/2

  m_enable_DJN = true;
}

std::vector<BigNumber> PublicKey::getDJNObfuscator(std::size_t sz) const {
  std::vector<BigNumber> r(sz);
  std::vector<BigNumber> base(sz, m_hs);
  std::vector<BigNumber> sq(sz, *m_nsquare);

  if (m_testv) {
    r = m_r;
  } else {
    for (auto& r_ : r) {
      r_ = getRandomBN(m_randbits);
    }
  }
  return modExp(base, r, sq);
}

std::vector<BigNumber> PublicKey::getNormalObfuscator(std::size_t sz) const {
  std::vector<BigNumber> r(sz);
  std::vector<BigNumber> sq(sz, *m_nsquare);
  std::vector<BigNumber> pown(sz, *m_n);

  if (m_testv) {
    r = m_r;
  } else {
    for (int i = 0; i < sz; i++) {
      r[i] = getRandomBN(m_bits);
      r[i] = r[i] % (*m_n - 1) + 1;
    }
  }
  return modExp(r, pown, sq);
}

void PublicKey::applyObfuscator(std::vector<BigNumber>& ciphertext) const {
  std::size_t sz = ciphertext.size();
  std::vector<BigNumber> obfuscator =
      m_enable_DJN ? getDJNObfuscator(sz) : getNormalObfuscator(sz);
  BigNumber sq = *m_nsquare;

  for (std::size_t i = 0; i < sz; ++i)
    ciphertext[i] = sq.ModMul(ciphertext[i], obfuscator[i]);
}

void PublicKey::setRandom(const std::vector<BigNumber>& r) {
  std::copy(r.begin(), r.end(), std::back_inserter(m_r));
  m_testv = true;
}

void PublicKey::setHS(const BigNumber& hs) { m_hs = hs; }

std::vector<BigNumber> PublicKey::raw_encrypt(const std::vector<BigNumber>& pt,
                                              bool make_secure) const {
  std::size_t pt_size = pt.size();

  std::vector<BigNumber> ct(pt_size);

  for (std::size_t i = 0; i < pt_size; i++)
    ct[i] = (*m_n * pt[i] + 1) % (*m_nsquare);

  if (make_secure) applyObfuscator(ct);

  return ct;
}

CipherText PublicKey::encrypt(const PlainText& pt, bool make_secure) const {
  ERROR_CHECK(m_isInitialized, "encrypt: Public key is NOT initialized.");

  std::size_t pt_size = pt.getSize();
  ERROR_CHECK(pt_size > 0, "encrypt: Cannot encrypt empty PlainText");
  std::vector<BigNumber> ct_bn_v(pt_size);

  // If hybrid OPTIMAL mode is used, use a special ratio
  if (isHybridOptimal()) {
    float qat_ratio = (pt_size <= IPCL_WORKLOAD_SIZE_THRESHOLD)
                          ? IPCL_HYBRID_MODEXP_RATIO_FULL
                          : IPCL_HYBRID_MODEXP_RATIO_ENCRYPT;
    setHybridRatio(qat_ratio, false);
  }

  ct_bn_v = raw_encrypt(pt.getTexts(), make_secure);
  return CipherText(this, ct_bn_v);
}

void PublicKey::setDJN(const BigNumber& hs, int randbit) {
  if (m_enable_DJN) return;

  m_hs = hs;
  m_randbits = randbit;
  m_enable_DJN = true;
}

void PublicKey::create(const BigNumber& n, int bits, bool enableDJN_) {
  m_n = std::make_shared<BigNumber>(n);
  m_g = std::make_shared<BigNumber>(*m_n + 1);
  m_nsquare = std::make_shared<BigNumber>((*m_n) * (*m_n));
  m_bits = bits;
  m_dwords = BITSIZE_DWORD(m_bits * 2);
  m_enable_DJN = enableDJN_;
  if (enableDJN_) {
    this->enableDJN();
  } else {
    m_hs = BigNumber::Zero();
    m_randbits = 0;
  }
  m_testv = false;
  m_isInitialized = true;
  std::cout << "create complete" << std::endl;
}

void PublicKey::create(const BigNumber& n, int bits, const BigNumber& hs,
                       int randbits) {
  create(n, bits, false);  // set DJN to false and manually set
  m_hs = hs;
  m_randbits = randbits;
}

}  // namespace ipcl
