// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/paillier_prikey.hpp"

#include <crypto_mb/exp.h>

#include <cstring>

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

PaillierPrivateKey::PaillierPrivateKey(PaillierPublicKey* public_key,
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
      m_x(m_n.InverseMul(m_pubkey->ippMontExp(m_g, m_lambda, m_nsquare) - 1) /
          m_n),
      m_bits(m_pubkey->getBits()),
      m_dwords(m_pubkey->getDwords()),
      m_enable_crt(true) {
  if (p * q != m_n) {
    throw std::runtime_error("Public key does not match p * q.");
  }

  if (p == q) {
    throw std::runtime_error("p and q error.");
  }
}

void PaillierPrivateKey::decryptRAW(BigNumber plaintext[8],
                                    const BigNumber ciphertext[8]) {
  mbx_status st = MBX_STATUS_OK;

  // setup buffer for mbx_exp
  int bufferLen = mbx_exp_BufferSize(m_bits * 2);
  auto pBuffer = std::vector<Ipp8u>(bufferLen);

  std::vector<int64u*> out_m(8), cip_array(8);
  int length = m_dwords * sizeof(int64u);

  for (int i = 0; i < 8; i++) {
    out_m[i] = reinterpret_cast<int64u*>(alloca(length));
    cip_array[i] = reinterpret_cast<int64u*>(alloca(length));

    if (out_m[i] == nullptr || cip_array[i] == nullptr)
      throw std::runtime_error("decryptRAW: alloc memory for error");

    memset(out_m[i], 0, length);
    memset(cip_array[i], 0, length);
  }

  // TODO(bwang30): add multi-buffered modular exponentiation
  std::vector<Ipp32u*> pow_c(8), pow_nsquare(8), pow_lambda(8);

  int cBitLen, lBitLen, nsqBitLen;
  BigNumber lambda = m_lambda;

  for (int i = 0; i < 8; i++) {
    ippsRef_BN(nullptr, &cBitLen, reinterpret_cast<Ipp32u**>(&pow_c[i]),
               ciphertext[i]);
    ippsRef_BN(nullptr, &lBitLen, &pow_lambda[i], lambda);
    ippsRef_BN(nullptr, &nsqBitLen, &pow_nsquare[i], m_nsquare);

    memcpy(cip_array[i], pow_c[i], BITSIZE_WORD(cBitLen) * 4);
  }

  st = mbx_exp_mb8(out_m.data(), cip_array.data(),
                   reinterpret_cast<Ipp64u**>(pow_lambda.data()), lBitLen,
                   reinterpret_cast<Ipp64u**>(pow_nsquare.data()), nsqBitLen,
                   pBuffer.data(), bufferLen);

  for (int i = 0; i < 8; i++) {
    if (MBX_STATUS_OK != MBX_GET_STS(st, i))
      throw std::runtime_error(
          std::string(
              "decryptRAW: error multi buffered exp modules, error code = ") +
          std::to_string(MBX_GET_STS(st, i)));
  }

  BigNumber ipp_res(m_nsquare);
  BigNumber nn = m_n;
  BigNumber xx = m_x;

  for (int i = 0; i < 8; i++) {
    ipp_res.Set(reinterpret_cast<Ipp32u*>(out_m[i]), BITSIZE_WORD(nsqBitLen),
                IppsBigNumPOS);
    BigNumber m = (ipp_res - 1) / nn;
    m = m * xx;
    plaintext[i] = m % nn;
  }
}

void PaillierPrivateKey::decrypt(BigNumber plaintext[8],
                                 const BigNumber ciphertext[8]) {
  if (m_enable_crt)
    decryptCRT(plaintext, ciphertext);
  else
    decryptRAW(plaintext, ciphertext);
}

void PaillierPrivateKey::decrypt(BigNumber plaintext[8],
                                 const PaillierEncryptedNumber ciphertext) {
  // check key match
  if (ciphertext.getPK().getN() != m_pubkey->getN())
    throw std::runtime_error("decrypt: public key mismatch error.");

  std::vector<BigNumber> res(8);
  ciphertext.getArrayBN(res.data());
  if (m_enable_crt)
    decryptCRT(plaintext, res.data());
  else
    decryptRAW(plaintext, res.data());
}

// CRT to calculate base^exp mod n^2
void PaillierPrivateKey::decryptCRT(BigNumber plaintext[8],
                                    const BigNumber ciphertext[8]) {
  std::vector<BigNumber> resp(8), resq(8);
  std::vector<BigNumber> basep(8), baseq(8);
  std::vector<BigNumber> pm1(8, m_pminusone), qm1(8, m_qminusone);
  std::vector<BigNumber> psq(8, m_psquare), qsq(8, m_qsquare);

  for (int i = 0; i < 8; i++) {
    basep[i] = ciphertext[i] % psq[i];
    baseq[i] = ciphertext[i] % qsq[i];
  }

  // Based on the fact a^b mod n = (a mod n)^b mod n
  m_pubkey->ippMultiBuffExp(resp.data(), basep.data(), pm1.data(), psq.data());
  m_pubkey->ippMultiBuffExp(resq.data(), baseq.data(), qm1.data(), qsq.data());

  for (int i = 0; i < 8; i++) {
    BigNumber dp = computeLfun(resp[i], m_p) * m_hp % m_p;
    BigNumber dq = computeLfun(resq[i], m_q) * m_hq % m_q;
    plaintext[i] = computeCRT(dp, dq);
  }
}

BigNumber PaillierPrivateKey::computeCRT(const BigNumber& mp,
                                         const BigNumber& mq) {
  BigNumber u = (mq - mp) * m_pinverse % m_q;
  return mp + (u * m_p);
}

BigNumber PaillierPrivateKey::computeLfun(const BigNumber& a,
                                          const BigNumber& b) {
  return (a - 1) / b;
}

BigNumber PaillierPrivateKey::computeHfun(const BigNumber& a,
                                          const BigNumber& b) {
  // Based on the fact a^b mod n = (a mod n)^b mod n
  BigNumber xm = a - 1;
  BigNumber base = m_g % b;
  BigNumber pm = m_pubkey->ippMontExp(base, xm, b);
  BigNumber lcrt = computeLfun(pm, a);
  return a.InverseMul(lcrt);
}

}  // namespace ipcl
