// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/pub_key.hpp"

#include <crypto_mb/exp.h>

#include <algorithm>
#include <climits>
#include <cstring>
#include <random>

#include "ipcl/ciphertext.hpp"
#include "ipcl/mod_exp.hpp"
#include "ipcl/util.hpp"

namespace ipcl {

static inline auto randomUniformUnsignedInt() {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);
  return dist(rng);
}

PublicKey::PublicKey(const BigNumber& n, int bits, bool enableDJN_)
    : m_n(n),
      m_g(n + 1),
      m_nsquare(n * n),
      m_bits(bits),
      m_dwords(BITSIZE_DWORD(bits * 2)),
      m_init_seed(randomUniformUnsignedInt()),
      m_enable_DJN(false),
      m_testv(false) {
  if (enableDJN_) this->enableDJN();  // sets m_enable_DJN
}

void PublicKey::enableDJN() {
  BigNumber gcd;
  BigNumber rmod;
  do {
    int rand_bit = m_n.BitSize();
    BigNumber rand = getRandomBN(rand_bit + 128);
    rmod = rand % m_n;
    gcd = rand.gcd(m_n);
  } while (gcd.compare(1));

  BigNumber rmod_sq = rmod * rmod;
  BigNumber rmod_neg = rmod_sq * -1;
  BigNumber h = rmod_neg % m_n;
  m_hs = ipcl::ippModExp(h, m_n, m_nsquare);
  m_randbits = m_bits >> 1;  // bits/2

  m_enable_DJN = true;
}

std::vector<BigNumber> PublicKey::getDJNObfuscator(std::size_t sz) const {
  std::vector<BigNumber> r(sz);
  std::vector<BigNumber> base(sz, m_hs);
  std::vector<BigNumber> sq(sz, m_nsquare);

  if (m_testv) {
    r = m_r;
  } else {
    for (auto& r_ : r) {
      r_ = getRandomBN(m_randbits);
    }
  }
  return ipcl::ippModExp(base, r, sq);
}

std::vector<BigNumber> PublicKey::getNormalObfuscator(std::size_t sz) const {
  std::vector<BigNumber> r(sz);
  std::vector<BigNumber> sq(sz, m_nsquare);
  std::vector<BigNumber> pown(sz, m_n);

  if (m_testv) {
    r = m_r;
  } else {
    for (int i = 0; i < sz; i++) {
      r[i] = getRandomBN(m_bits);
      r[i] = r[i] % (m_n - 1) + 1;
    }
  }
  return ipcl::ippModExp(r, pown, sq);
}

void PublicKey::applyObfuscator(std::vector<BigNumber>& ciphertext) const {
  std::size_t sz = ciphertext.size();
  std::vector<BigNumber> obfuscator =
      m_enable_DJN ? getDJNObfuscator(sz) : getNormalObfuscator(sz);
  BigNumber sq = m_nsquare;

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
    ct[i] = (m_n * pt[i] + 1) % m_nsquare;

  if (make_secure) applyObfuscator(ct);

  return ct;
}

CipherText PublicKey::encrypt(const PlainText& pt, bool make_secure) const {
  std::size_t pt_size = pt.getSize();
  ERROR_CHECK(pt_size > 0, "encrypt: Cannot encrypt empty PlainText");
  std::vector<BigNumber> ct_bn_v(pt_size);

  ct_bn_v = raw_encrypt(pt.getTexts(), make_secure);
  return CipherText(this, ct_bn_v);
}

void PublicKey::setDJN(const BigNumber& hs, int randbit) {
  if (m_enable_DJN) return;

  m_hs = hs;
  m_randbits = randbit;
  m_enable_DJN = true;
}
}  // namespace ipcl
