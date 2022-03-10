// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/paillier_prikey.hpp"

#include <crypto_mb/exp.h>

#include <cstring>

#include "ipcl/mod_exp.hpp"
#include "ipcl/util.hpp"

namespace ipcl {
/**
 * Compute lcm for p and q
 * @param[in] p p - 1 of private key
 * @param[in] q q - 1 of private key
 * @return the lcm result of type BigNumber
 */
static inline BigNumber lcm(const BigNumber& p, const BigNumber& q) {
  BigNumber gcd(p);
  ippsGcd_BN(BN(p), BN(q), BN(gcd));
  return p * q / gcd;
}

PaillierPrivateKey::PaillierPrivateKey(const PaillierPublicKey* public_key,
                                       const BigNumber& p, const BigNumber& q)
    : m_pubkey(public_key),
      m_n(m_pubkey->getN()),
      m_nsquare(m_pubkey->getNSQ()),
      m_g(m_pubkey->getG()),
      m_p((q < p) ? q : p),
      m_q((q < p) ? p : q),
      m_pminusone(m_p - 1),
      m_qminusone(m_q - 1),
      m_psquare(m_p * m_p),
      m_qsquare(m_q * m_q),
      m_pinverse(m_q.InverseMul(m_p)),
      m_hp(computeHfun(m_p, m_psquare)),
      m_hq(computeHfun(m_q, m_qsquare)),
      // lcm(P-1,Q-1) = (P-1)*(Q-1)/gcd(P-1,Q-1), gcd in ipp-crypto is
      // ippsGcd_BN
      m_lambda(lcm(m_pminusone, m_qminusone)),
      // TODO(bwang30): check if ippsModInv_BN does the same thing with
      // mpz_invert
      m_x(m_n.InverseMul((ipcl::ippModExp(m_g, m_lambda, m_nsquare) - 1) /
                         m_n)),
      m_bits(m_pubkey->getBits()),
      m_dwords(m_pubkey->getDwords()),
      m_enable_crt(true) {
  ERROR_CHECK(p * q == m_n,
              "PaillierPrivateKey ctor: Public key does not match p * q.");
  ERROR_CHECK(p != q, "PaillierPrivateKey ctor: p and q are same");
}

void PaillierPrivateKey::decryptRAW(
    std::vector<BigNumber>& plaintext,
    const std::vector<BigNumber>& ciphertext) const {
  std::vector<BigNumber> pow_lambda(IPCL_CRYPTO_MB_SIZE, m_lambda);
  std::vector<BigNumber> modulo(IPCL_CRYPTO_MB_SIZE, m_nsquare);
  std::vector<BigNumber> res = ipcl::ippModExp(ciphertext, pow_lambda, modulo);

  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    BigNumber m = (res[i] - 1) / m_n;
    m *= m_x;
    plaintext[i] = m % m_n;
  }
}

void PaillierPrivateKey::decrypt(
    std::vector<BigNumber>& plaintext,
    const std::vector<BigNumber>& ciphertext) const {
  VEC_SIZE_CHECK(plaintext);
  VEC_SIZE_CHECK(ciphertext);

  if (m_enable_crt)
    decryptCRT(plaintext, ciphertext);
  else
    decryptRAW(plaintext, ciphertext);
}

void PaillierPrivateKey::decrypt(
    std::vector<BigNumber>& plaintext,
    const PaillierEncryptedNumber ciphertext) const {
  VEC_SIZE_CHECK(plaintext);
  // check key match
  ERROR_CHECK(ciphertext.getPK().getN() == m_pubkey->getN(),
              "decrypt: public key mismatch error.");

  const std::vector<BigNumber>& res = ciphertext.getArrayBN();
  if (m_enable_crt)
    decryptCRT(plaintext, res);
  else
    decryptRAW(plaintext, res);
}

// CRT to calculate base^exp mod n^2
void PaillierPrivateKey::decryptCRT(
    std::vector<BigNumber>& plaintext,
    const std::vector<BigNumber>& ciphertext) const {
  std::vector<BigNumber> basep(IPCL_CRYPTO_MB_SIZE), baseq(IPCL_CRYPTO_MB_SIZE);
  std::vector<BigNumber> pm1(IPCL_CRYPTO_MB_SIZE, m_pminusone),
      qm1(IPCL_CRYPTO_MB_SIZE, m_qminusone);
  std::vector<BigNumber> psq(IPCL_CRYPTO_MB_SIZE, m_psquare),
      qsq(IPCL_CRYPTO_MB_SIZE, m_qsquare);

  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    basep[i] = ciphertext[i] % psq[i];
    baseq[i] = ciphertext[i] % qsq[i];
  }

  // Based on the fact a^b mod n = (a mod n)^b mod n
  std::vector<BigNumber> resp = ipcl::ippModExp(basep, pm1, psq);
  std::vector<BigNumber> resq = ipcl::ippModExp(baseq, qm1, qsq);

  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    BigNumber dp = computeLfun(resp[i], m_p) * m_hp % m_p;
    BigNumber dq = computeLfun(resq[i], m_q) * m_hq % m_q;
    plaintext[i] = computeCRT(dp, dq);
  }
}

BigNumber PaillierPrivateKey::computeCRT(const BigNumber& mp,
                                         const BigNumber& mq) const {
  BigNumber u = (mq - mp) * m_pinverse % m_q;
  return mp + (u * m_p);
}

BigNumber PaillierPrivateKey::computeLfun(const BigNumber& a,
                                          const BigNumber& b) const {
  return (a - 1) / b;
}

BigNumber PaillierPrivateKey::computeHfun(const BigNumber& a,
                                          const BigNumber& b) const {
  // Based on the fact a^b mod n = (a mod n)^b mod n
  BigNumber xm = a - 1;
  BigNumber base = m_g % b;
  BigNumber pm = ipcl::ippModExp(base, xm, b);
  BigNumber lcrt = computeLfun(pm, a);
  return a.InverseMul(lcrt);
}

}  // namespace ipcl
