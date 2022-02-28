// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_PRI_KEY_HPP_
#define IPCL_INCLUDE_IPCL_PRI_KEY_HPP_

#include "ipcl/bignum.h"
#include "ipcl/paillier_ops.hpp"
#include "ipcl/pub_key.hpp"

class PaillierPrivateKey {
 public:
  /**
   * PaillierPrivateKey construct function
   * @param[in] public_key paillier public key
   * @param[in] p p of private key in paillier paper
   * @param[in] q q of private key in paillier paper
   */
  PaillierPrivateKey(PaillierPublicKey* public_key, const BigNumber& p,
                     const BigNumber& q);

  /**
   * Chinese-remaindering enabling
   * @param[in] crt Apply Chinese remaindering theorem
   */
  void enableCRT(bool crt) { m_enable_crt = crt; }

  /**
   * Decrypt ciphtext
   * @param[out] plaintext output of the decryption
   * @param[in] ciphertext ciphertext to be decrypted
   */
  void decrypt(BigNumber plaintext[8], const BigNumber ciphertext[8]);

  /**
   * Decrypt ciphtext
   * @param[out] plaintext output of the decryption
   * @param[in] ciphertext PaillierEncryptedNumber to be decrypted
   */
  void decrypt(BigNumber plaintext[8],
               const PaillierEncryptedNumber ciphertext);

  const void* addr = static_cast<const void*>(this);

  /**
   * Get N of public key in paillier paper
   */
  BigNumber getN() const { return m_n; }

  /**
   * Get p of private key in paillier paper
   */
  BigNumber getP() const { return m_p; }

  /**
   * Get q of private key in paillier paper
   */
  BigNumber getQ() const { return m_q; }

  /**
   * Get bits of key
   */
  int getBits() const { return m_bits; }

  /**
   * Get public key
   */
  PaillierPublicKey* getPubKey() const { return m_pubkey; }

  /**
   * @brief Support function for ISO/IEC 18033-6 compliance check
   *
   * @return BigNumber
   */
  BigNumber getLambda() { return m_lambda; }

 private:
  PaillierPublicKey* m_pubkey;
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
   * Compute L function in paillier paper
   * @param[in] a input a
   * @param[in] b input b
   */
  BigNumber computeLfun(const BigNumber& a, const BigNumber& b);

  /**
   * Compute H function in paillier paper
   * @param[in] a input a
   * @param[in] b input b
   */
  BigNumber computeHfun(const BigNumber& a, const BigNumber& b);

  /**
   * Compute CRT function in paillier paper
   * @param[in] mp input mp
   * @param[in] mq input mq
   */
  BigNumber computeCRT(const BigNumber& mp, const BigNumber& mq);

  /**
   * Raw decryption function without CRT optimization
   * @param[out] plaintext output plaintext
   * @param[in] ciphertext input ciphertext
   */
  void decryptRAW(BigNumber plaintext[8], const BigNumber ciphertext[8]);

  /**
   * Raw decryption function with CRT optimization
   * @param[out] plaintext output plaintext
   * @param[in] ciphertext input ciphertext
   */
  void decryptCRT(BigNumber plaintext[8], const BigNumber ciphertext[8]);
};

#endif  // IPCL_INCLUDE_IPCL_PRI_KEY_HPP_
