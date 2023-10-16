// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_PRI_KEY_HPP_
#define IPCL_INCLUDE_IPCL_PRI_KEY_HPP_

#include <memory>
#include <utility>
#include <vector>

#include "ipcl/mod_exp.hpp"
#include "ipcl/ciphertext.hpp"
#include "ipcl/plaintext.hpp"

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

class PrivateKey {
 public:
  PrivateKey() = default;
  ~PrivateKey() = default;

  /**
   * PrivateKey constructor
   * @param[in] pk paillier public key
   * @param[in] p p of private key in paillier scheme
   * @param[in] q q of private key in paillier scheme
   */
  PrivateKey(const PublicKey& pk, const BigNumber& p, const BigNumber& q);

  /**
   * PrivateKey constructor
   * @param[in] pk paillier public key
   * @param[in] p p of private key in paillier scheme
   * @param[in] q q of private key in paillier scheme
   */
  PrivateKey(const BigNumber& n, const BigNumber& p, const BigNumber& q);

  /**
   * Enable Chinese Remainder Theorem
   * @param[in] crt Apply Chinese Remainder Theorem
   */
  void enableCRT(bool crt) { m_enable_crt = crt; }

  /**
   * Decrypt ciphertext
   * @param[in] ciphertext CipherText to be decrypted
   * @return plaintext of type PlainText
   */
  PlainText decrypt(const CipherText& ciphertext) const;

  const void* addr = static_cast<const void*>(this);

  /**
   * Get N of public key in paillier scheme
   */
  std::shared_ptr<BigNumber> getN() const { return m_n; }

  /**
   * Get p of private key in paillier scheme
   */
  std::shared_ptr<BigNumber> getP() const { return m_p; }

  /**
   * Get q of private key in paillier scheme
   */
  std::shared_ptr<BigNumber> getQ() const { return m_q; }

  /**
   * @brief Support function for ISO/IEC 18033-6 compliance check
   *
   * @return BigNumber
   */
  BigNumber getLambda() const { return m_lambda; }

  /**
   * Check whether priv key is initialized
   */
  bool isInitialized() { return m_isInitialized; }

 private:
  friend class cereal::access;
  template <class Archive>
  void save(Archive& ar, const Ipp32u version) const {
    ar(::cereal::make_nvp("bits", m_p->BitSize()));
    ar(::cereal::make_nvp("p", *m_p));
    ar(::cereal::make_nvp("q", *m_q));
  }

  template <class Archive>
  void load(Archive& ar, const Ipp32u version) {
    int bits;
    ar(::cereal::make_nvp("bits", bits));

    int bn_len = bits / 32;
    std::vector<Ipp32u> p_v(bn_len, 0);
    std::vector<Ipp32u> q_v(bn_len, 0);
    BigNumber p(p_v.data(), bn_len);
    BigNumber q(q_v.data(), bn_len);

    ar(::cereal::make_nvp("p", p));
    ar(::cereal::make_nvp("q", q));

    m_n = std::make_shared<BigNumber>(p * q);
    m_nsquare = std::make_shared<BigNumber>((*m_n) * (*m_n));
    m_g = std::make_shared<BigNumber>((*m_n) + 1);
    m_enable_crt = true;
    m_p = (q < p) ? std::make_shared<BigNumber>(q)
                  : std::make_shared<BigNumber>(p);
    m_q = (q < p) ? std::make_shared<BigNumber>(p)
                  : std::make_shared<BigNumber>(q);
    m_pminusone = *m_p - 1;
    m_qminusone = *m_q - 1;
    m_psquare = (*m_p) * (*m_p);
    m_qsquare = (*m_q) * (*m_q);
    m_pinverse = (*m_q).InverseMul(*m_p);
    m_hp = computeHfun(*m_p, m_psquare);
    m_hq = computeHfun(*m_q, m_qsquare);
    m_lambda = lcm(m_pminusone, m_qminusone);
    m_x = (*m_n).InverseMul((modExp(*m_g, m_lambda, *m_nsquare) - 1) / (*m_n));
    m_isInitialized = true;
  }

  bool m_isInitialized = false;
  bool m_enable_crt = false;

  std::shared_ptr<BigNumber> m_n;
  std::shared_ptr<BigNumber> m_nsquare;
  std::shared_ptr<BigNumber> m_g;
  std::shared_ptr<BigNumber> m_p;
  std::shared_ptr<BigNumber> m_q;

  BigNumber m_pminusone;
  BigNumber m_qminusone;
  BigNumber m_psquare;
  BigNumber m_qsquare;
  BigNumber m_pinverse;
  BigNumber m_hp;
  BigNumber m_hq;
  BigNumber m_lambda;
  BigNumber m_x;

  /**
   * Compute L function in paillier scheme
   * @param[in] a input a
   * @param[in] b input b
   * @return the L function result of type BigNumber
   */
  BigNumber computeLfun(const BigNumber& a, const BigNumber& b) const;

  /**
   * Compute H function in paillier scheme
   * @param[in] a input a
   * @param[in] b input b
   * @return the H function result of type BigNumber
   */
  BigNumber computeHfun(const BigNumber& a, const BigNumber& b) const;

  /**
   * Compute CRT function in paillier scheme
   * @param[in] mp input mp
   * @param[in] mq input mq
   * @return the CRT result of type BigNumber
   */
  BigNumber computeCRT(const BigNumber& mp, const BigNumber& mq) const;

  /**
   * Raw decryption function without CRT optimization
   * @param[out] plaintext output plaintext
   * @param[in] ciphertext input ciphertext
   */
  void decryptRAW(std::vector<BigNumber>& plaintext,
                  const std::vector<BigNumber>& ciphertext) const;

  /**
   * Raw decryption function with CRT optimization
   * @param[out] plaintext output plaintext
   * @param[in] ciphertext input ciphertext
   */
  void decryptCRT(std::vector<BigNumber>& plaintext,
                  const std::vector<BigNumber>& ciphertext) const;
};

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_PRI_KEY_HPP_
