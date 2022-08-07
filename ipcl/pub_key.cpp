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

void PublicKey::applyDjnObfuscator(std::vector<BigNumber>& obfuscator) const {
  std::size_t obf_size = obfuscator.size();
  std::vector<BigNumber> r(obf_size);
  std::vector<BigNumber> base(obf_size, m_hs);
  std::vector<BigNumber> sq(obf_size, m_nsquare);

  if (m_testv) {
    r = m_r;
  } else {
    for (auto& r_ : r) {
      r_ = getRandomBN(m_randbits);
    }
  }
  obfuscator = ipcl::ippModExp(base, r, sq);
}

void PublicKey::applyNormalObfuscator(
    std::vector<BigNumber>& obfuscator) const {
  std::size_t obf_size = obfuscator.size();
  std::vector<BigNumber> r(obf_size);
  std::vector<BigNumber> sq(obf_size, m_nsquare);
  std::vector<BigNumber> pown(obf_size, m_n);

  if (m_testv) {
    r = m_r;
  } else {
    for (int i = 0; i < obf_size; i++) {
      r[i] = getRandomBN(m_bits);
      r[i] = r[i] % (m_n - 1) + 1;
    }
  }
  obfuscator = ipcl::ippModExp(r, pown, sq);
}

void PublicKey::applyObfuscator(std::vector<BigNumber>& obfuscator) const {
  if (m_enable_DJN) {
    applyDjnObfuscator(obfuscator);
  } else {
    applyNormalObfuscator(obfuscator);
  }
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
  BigNumber sq = m_nsquare;

  for (std::size_t i = 0; i < pt_size; i++) {
    ct[i] = (m_n * pt[i] + 1) % m_nsquare;
  }

  if (make_secure) {
    std::vector<BigNumber> obfuscator(pt_size);
    applyObfuscator(obfuscator);

    for (std::size_t i = 0; i < pt_size; i++)
      ct[i] = sq.ModMul(ct[i], obfuscator[i]);
  }
  return ct;
}

CipherText PublicKey::encrypt(const PlainText& pt, bool make_secure) const {
  std::size_t pt_size = pt.getSize();
  ERROR_CHECK(pt_size > 0, "encrypt: Cannot encrypt emtpy PlainText");
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
