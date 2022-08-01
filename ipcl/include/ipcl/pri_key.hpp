// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_PRI_KEY_HPP_
#define IPCL_INCLUDE_IPCL_PRI_KEY_HPP_

#include <vector>

#include "ipcl/ciphertext.hpp"
#include "ipcl/plaintext.hpp"

namespace ipcl {

class PrivateKey {
 public:
  /**
   * PrivateKey constructor
   * @param[in] public_key paillier public key
   * @param[in] p p of private key in paillier scheme
   * @param[in] q q of private key in paillier scheme
   */
  PrivateKey(const PublicKey* public_key, const BigNumber& p,
             const BigNumber& q);

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
  BigNumber getN() const { return m_n; }

  /**
   * Get p of private key in paillier scheme
   */
  BigNumber getP() const { return m_p; }

  /**
   * Get q of private key in paillier scheme
   */
  BigNumber getQ() const { return m_q; }

  /**
   * Get bits of key
   */
  int getBits() const { return m_bits; }

  /**
   * Get public key
   */
  const PublicKey* getPubKey() const { return m_pubkey; }

  /**
   * @brief Support function for ISO/IEC 18033-6 compliance check
   *
   * @return BigNumber
   */
  BigNumber getLambda() const { return m_lambda; }

 private:
  const PublicKey* m_pubkey;
  BigNumber m_n;
  BigNumber m_nsquare;
  BigNumber m_g;
  BigNumber m_p;
  BigNumber m_q;
  BigNumber m_pminusone;
  BigNumber m_qminusone;
  BigNumber m_psquare;
  BigNumber m_qsquare;
  BigNumber m_pinverse;
  BigNumber m_hp;
  BigNumber m_hq;
  BigNumber m_lambda;
  BigNumber m_x;
  int m_bits;
  int m_dwords;
  bool m_enable_crt;

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
