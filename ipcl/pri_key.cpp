// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/pri_key.hpp"

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

PrivateKey::PrivateKey(const PublicKey* public_key, const BigNumber& p,
                       const BigNumber& q)
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
              "PrivateKey ctor: Public key does not match p * q.");
  ERROR_CHECK(p != q, "PrivateKey ctor: p and q are same");
}

PlainText PrivateKey::decrypt(const CipherText& ct) const {
  ERROR_CHECK(ct.getPubKey()->getN() == m_pubkey->getN(),
              "decrypt: The value of N in public key mismatch.");

  std::size_t ct_size = ct.getSize();
  ERROR_CHECK(ct_size > 0, "decrypt: Cannot decrypt empty CipherText");

  std::vector<BigNumber> pt_bn(ct_size);
  std::vector<BigNumber> ct_bn = ct.getTexts();

  if (m_enable_crt)
    decryptCRT(pt_bn, ct_bn);
  else
    decryptRAW(pt_bn, ct_bn);

  return PlainText(pt_bn);
}

void PrivateKey::decryptRAW(std::vector<BigNumber>& plaintext,
                            const std::vector<BigNumber>& ciphertext) const {
  std::size_t v_size = plaintext.size();

  std::vector<BigNumber> pow_lambda(v_size, m_lambda);
  std::vector<BigNumber> modulo(v_size, m_nsquare);
  std::vector<BigNumber> res = ipcl::ippModExp(ciphertext, pow_lambda, modulo);

#ifdef IPCL_USE_OMP
  int omp_remaining_threads = OMPUtilities::MaxThreads;
#pragma omp parallel for num_threads( \
    OMPUtilities::assignOMPThreads(omp_remaining_threads, v_size))
#endif  // IPCL_USE_OMP
  for (int i = 0; i < v_size; i++) {
    BigNumber nn = m_n;
    BigNumber xx = m_x;
    BigNumber m = ((res[i] - 1) / nn) * xx;
    plaintext[i] = m % nn;
  }
}

// CRT to calculate base^exp mod n^2
void PrivateKey::decryptCRT(std::vector<BigNumber>& plaintext,
                            const std::vector<BigNumber>& ciphertext) const {
  std::size_t v_size = plaintext.size();

  std::vector<BigNumber> basep(v_size), baseq(v_size);
  std::vector<BigNumber> pm1(v_size, m_pminusone), qm1(v_size, m_qminusone);
  std::vector<BigNumber> psq(v_size, m_psquare), qsq(v_size, m_qsquare);

#ifdef IPCL_USE_OMP
  int omp_remaining_threads = OMPUtilities::MaxThreads;
#pragma omp parallel for num_threads( \
    OMPUtilities::assignOMPThreads(omp_remaining_threads, v_size))
#endif  // IPCL_USE_OMP
  for (int i = 0; i < v_size; i++) {
    basep[i] = ciphertext[i] % psq[i];
    baseq[i] = ciphertext[i] % qsq[i];
  }

  // Based on the fact a^b mod n = (a mod n)^b mod n
  std::vector<BigNumber> resp = ipcl::ippModExp(basep, pm1, psq);
  std::vector<BigNumber> resq = ipcl::ippModExp(baseq, qm1, qsq);

#ifdef IPCL_USE_OMP
  omp_remaining_threads = OMPUtilities::MaxThreads;
#pragma omp parallel for num_threads( \
    OMPUtilities::assignOMPThreads(omp_remaining_threads, v_size))
#endif  // IPCL_USE_OMP
  for (int i = 0; i < v_size; i++) {
    BigNumber dp = computeLfun(resp[i], m_p) * m_hp % m_p;
    BigNumber dq = computeLfun(resq[i], m_q) * m_hq % m_q;
    plaintext[i] = computeCRT(dp, dq);
  }
}

BigNumber PrivateKey::computeCRT(const BigNumber& mp,
                                 const BigNumber& mq) const {
  BigNumber u = (mq - mp) * m_pinverse % m_q;
  return mp + (u * m_p);
}

BigNumber PrivateKey::computeLfun(const BigNumber& a,
                                  const BigNumber& b) const {
  return (a - 1) / b;
}

BigNumber PrivateKey::computeHfun(const BigNumber& a,
                                  const BigNumber& b) const {
  // Based on the fact a^b mod n = (a mod n)^b mod n
  BigNumber xm = a - 1;
  BigNumber base = m_g % b;
  BigNumber pm = ipcl::ippModExp(base, xm, b);
  BigNumber lcrt = computeLfun(pm, a);
  return a.InverseMul(lcrt);
}

}  // namespace ipcl
