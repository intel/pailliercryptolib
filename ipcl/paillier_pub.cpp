// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <crypto_mb/exp.h>

#include <climits>
#include <cstring>
#include <random>

#include "ipcl/pub_key.hpp"

static inline auto randomUniformUnsignedInt() {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);
  return dist(rng);
}

PaillierPublicKey::PaillierPublicKey(const BigNumber& n, int bits,
                                     bool enableDJN_)
    : m_n(n),
      m_g(n + 1),
      m_nsquare(n * n),
      m_bits(bits),
      m_dwords(BITSIZE_DWORD(bits * 2)),
      m_init_seed(randomUniformUnsignedInt()),
      m_enable_DJN(false),
      m_testv(false) {
  if (enableDJN_) this->enableDJN();  // this sets m_enable_DJN
}

// array of 32-bit random, using rand() from stdlib
Ipp32u* PaillierPublicKey::randIpp32u(Ipp32u* addr, int size) {
  for (int i = 0; i < size; i++)
    addr[i] = (rand_r(&m_init_seed) << 16) + rand_r(&m_init_seed);
  return addr;
}

// length is Arbitery
BigNumber PaillierPublicKey::getRandom(int length) {
  int size;
  int seedBitSize = 160;
  int seedSize = BITSIZE_WORD(seedBitSize);
  Ipp32u* seed = reinterpret_cast<Ipp32u*>(alloca(seedSize * sizeof(Ipp32u)));
  if (nullptr == seed) throw std::runtime_error("error alloc memory");
  ippsPRNGGetSize(&size);
  IppsPRNGState* pRand = reinterpret_cast<IppsPRNGState*>(alloca(size));
  if (nullptr == pRand) throw std::runtime_error("error alloc memory");
  ippsPRNGInit(seedBitSize, pRand);

  Ipp32u* addr = randIpp32u(seed, seedSize);
  BigNumber bseed(addr, seedSize, IppsBigNumPOS);

  ippsPRNGSetSeed(BN(bseed), pRand);

  // define length Big Numbers
  int bn_size = BITSIZE_WORD(length);
  ippsBigNumGetSize(bn_size, &size);
  IppsBigNumState* pBN = reinterpret_cast<IppsBigNumState*>(alloca(size));
  if (nullptr == pBN) throw std::runtime_error("error alloc memory");
  ippsBigNumInit(bn_size, pBN);

  int bnBitSize = length;
  ippsPRNGenRDRAND_BN(pBN, bnBitSize, pRand);
  BigNumber rand(pBN);
  return rand;
}

void PaillierPublicKey::enableDJN() {
  BigNumber gcd;
  BigNumber rmod;
  do {
    int rand_bit = m_n.BitSize();
    BigNumber rand = getRandom(rand_bit + 128);
    rmod = rand % m_n;
    gcd = rand.gcd(m_n);
  } while (gcd.compare(1));

  BigNumber rmod_sq = rmod * rmod;
  BigNumber rmod_neg = rmod_sq * -1;
  BigNumber h = rmod_neg % m_n;
  m_hs = ippMontExp(h, m_n, m_nsquare);
  m_randbits = m_bits >> 1;  // bits/2

  m_enable_DJN = true;
}

void PaillierPublicKey::apply_obfuscator(BigNumber obfuscator[8]) {
  BigNumber r[8];
  BigNumber pown[8];
  BigNumber base[8];
  BigNumber sq[8];

  if (m_enable_DJN) {
    for (int i = 0; i < 8; i++) {
      r[i] = getRandom(m_randbits);
      base[i] = m_hs;
      sq[i] = m_nsquare;
    }

    ippMultiBuffExp(obfuscator, base, r, sq);
  } else {
    for (int i = 0; i < 8; i++) {
      if (m_testv) {
        r[i] = m_r[i];
      } else {
        r[i] = getRandom(m_bits);
        r[i] = r[i] % (m_n - 1) + 1;
      }
      pown[i] = m_n;
      sq[i] = m_nsquare;
    }
    ippMultiBuffExp(obfuscator, r, pown, sq);
  }
}

void PaillierPublicKey::raw_encrypt(BigNumber ciphertext[8],
                                    const BigNumber plaintext[8],
                                    bool make_secure) {
  // Based on the fact that: (n+1)^plaintext mod n^2 = n*plaintext + 1 mod n^2
  BigNumber sq = m_nsquare;
  for (int i = 0; i < 8; i++) {
    BigNumber bn(plaintext[i]);
    ciphertext[i] = (m_n * bn + 1) % sq;
  }

  if (make_secure) {
    BigNumber obfuscator[8];
    apply_obfuscator(obfuscator);

    for (int i = 0; i < 8; i++)
      ciphertext[i] = sq.ModMul(ciphertext[i], obfuscator[i]);
  }
}

void PaillierPublicKey::encrypt(BigNumber ciphertext[8],
                                const BigNumber value[8], bool make_secure) {
  raw_encrypt(ciphertext, value, make_secure);
}

// Used for CT+PT, where PT do not need to add obfuscator
void PaillierPublicKey::encrypt(BigNumber& ciphertext, const BigNumber& value) {
  // Based on the fact that: (n+1)^plaintext mod n^2 = n*plaintext + 1 mod n^2
  BigNumber bn = value;
  BigNumber sq = m_nsquare;
  ciphertext = (m_n * bn + 1) % sq;

  /*----- Path to caculate (n+1)^plaintext mod n^2 ------------
  BigNumber bn(value);
  ciphertext = ippMontExp(m_g, bn, m_nsquare);
  ---------------------------------------------------------- */
}

void PaillierPublicKey::ippMultiBuffExp(BigNumber res[8], BigNumber base[8],
                                        const BigNumber pow[8],
                                        BigNumber m[8]) {
  mbx_status st = MBX_STATUS_OK;

  // Memory used for multi-buffered modular exponentiation
  Ipp64u* out_x[8];

  int64u* b_array[8];
  int64u* p_array[8];

  int bufferLen;
  Ipp8u* pBuffer;

  // setup buffer for mbx_exp
  int bits = m[0].BitSize();
  int dwords = BITSIZE_DWORD(bits);
  bufferLen = mbx_exp_BufferSize(bits);
  pBuffer = reinterpret_cast<Ipp8u*>(alloca(bufferLen));
  if (nullptr == pBuffer) throw std::runtime_error("error alloc memory");

  int length = dwords * sizeof(int64u);
  for (int i = 0; i < 8; i++) {
    out_x[i] = reinterpret_cast<int64u*>(alloca(length));

    b_array[i] = reinterpret_cast<int64u*>(alloca(length));
    p_array[i] = reinterpret_cast<int64u*>(alloca(length));

    if (nullptr == out_x[i] || nullptr == b_array[i] || nullptr == p_array[i])
      throw std::runtime_error("error alloc memory");
  }

  for (int i = 0; i < 8; i++) {
    memset(b_array[i], 0, dwords * 8);
    memset(p_array[i], 0, dwords * 8);
  }

  Ipp32u *pow_b[8], *pow_p[8], *pow_nsquare[8];
  int bBitLen, pBitLen, nsqBitLen;
  int expBitLen = 0;
  for (int i = 0; i < 8; i++) {
    ippsRef_BN(nullptr, &bBitLen, &pow_b[i], base[i]);
    ippsRef_BN(nullptr, &pBitLen, &pow_p[i], pow[i]);
    ippsRef_BN(nullptr, &nsqBitLen, &pow_nsquare[i], m[i]);

    memcpy(b_array[i], pow_b[i], BITSIZE_WORD(bBitLen) * 4);
    memcpy(p_array[i], pow_p[i], BITSIZE_WORD(pBitLen) * 4);
    if (expBitLen < pBitLen) expBitLen = pBitLen;
  }

  /*
   *Note: If actual sizes of exp are different, set the exp_bits parameter equal
   *to maximum size of the actual module in bit size and extend all the modules
   *with zero bits
   */
  st = mbx_exp_mb8(out_x, b_array, p_array, expBitLen,
                   reinterpret_cast<Ipp64u**>(pow_nsquare), nsqBitLen, pBuffer,
                   bufferLen);

  for (int i = 0; i < 8; i++) {
    if (MBX_STATUS_OK != MBX_GET_STS(st, i))
      throw std::runtime_error(
          std::string("error multi buffered exp modules, error code = ") +
          std::to_string(MBX_GET_STS(st, i)));
  }

  // It is important to hold a copy of nsquare for thread-safe purpose
  BigNumber bn_c(m[0]);
  for (int i = 0; i < 8; i++) {
    bn_c.Set(reinterpret_cast<Ipp32u*>(out_x[i]), BITSIZE_WORD(nsqBitLen),
             IppsBigNumPOS);

    res[i] = bn_c;
  }
}

BigNumber PaillierPublicKey::ippMontExp(const BigNumber& base,
                                        const BigNumber& pow,
                                        const BigNumber& m) {
  IppStatus st = ippStsNoErr;
  // It is important to declear res * bform bit length refer to ipp-crypto spec:
  // R should not be less than the data length of the modulus m
  BigNumber res(m);

  int bnBitLen;
  Ipp32u* pow_m;
  ippsRef_BN(nullptr, &bnBitLen, &pow_m, BN(m));
  int nlen = BITSIZE_WORD(bnBitLen);

  int size;
  // define and initialize Montgomery Engine over Modulus N
  ippsMontGetSize(IppsBinaryMethod, nlen, &size);
  IppsMontState* pMont = reinterpret_cast<IppsMontState*>(new Ipp8u[size]);
  if (nullptr == pMont) throw std::runtime_error("error alloc memory");
  ippsMontInit(IppsBinaryMethod, nlen, pMont);
  ippsMontSet(pow_m, nlen, pMont);

  // encode base into Montfomery form
  BigNumber bform(m);
  ippsMontForm(BN(base), pMont, BN(bform));

  // compute R = base^pow mod N
  st = ippsMontExp(BN(bform), BN(pow), pMont, BN(res));
  if (ippStsNoErr != st)
    throw std::runtime_error(std::string("ippsMontExp error code = ") +
                             std::to_string(st));

  BigNumber one(1);
  st = ippsMontMul(BN(res), BN(one), pMont, BN(res));  // R = MontMul(R,1)
  if (ippStsNoErr != st)
    throw std::runtime_error(std::string("ippsMontMul error code = ") +
                             std::to_string(st));

  delete[] reinterpret_cast<Ipp8u*>(pMont);
  return res;
}

BigNumber PaillierPublicKey::IPP_invert(BigNumber a, BigNumber b) {
  return b.InverseMul(a);
}
