// Copyright (C) 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_PAILLIER_OPS_HPP_
#define IPCL_INCLUDE_IPCL_PAILLIER_OPS_HPP_

#include <algorithm>

#include "ipcl/pub_key.hpp"

class PaillierEncryptedNumber {
 public:
  /**
   * PaillierEncryptedNumber construct function
   * @param[in] pub_key paillier public key
   * @param[in] bn ciphtext encrypted by paillier
   */
  PaillierEncryptedNumber(PaillierPublicKey* pub_key, const BigNumber& bn);

  /**
   * PaillierEncryptedNumber construct function
   * @param[in] pub_key paillier public key
   * @param[in] bn array of ciphtext encrypted by paillier
   * @param[in] length elements of array
   */
  PaillierEncryptedNumber(PaillierPublicKey* pub_key, const BigNumber bn[8],
                          size_t length = 8);

  /**
   * PaillierEncryptedNumber construct function
   * @param[in] pub_key paillier public key
   * @param[in] scalar array of scalar
   * integer)
   * @param[in] length elements of array
   */
  PaillierEncryptedNumber(PaillierPublicKey* pub_key, const uint32_t scalar[8],
                          size_t length = 8);

  /**
   * Arithmetic addition operator
   * @param[in] bn augend
   */
  PaillierEncryptedNumber operator+(const PaillierEncryptedNumber& bn) const;

  /**
   * Arithmetic addition operator
   * @param[in] other augend
   */
  PaillierEncryptedNumber operator+(const BigNumber& other) const;

  /**
   * Arithmetic addition operator
   * @param[in] other array of augend
   */
  PaillierEncryptedNumber operator+(const BigNumber other[8]) const;

  /**
   * Arithmetic multiply operator
   * @param[in] bn multiplicand
   */
  PaillierEncryptedNumber operator*(const PaillierEncryptedNumber& bn) const;

  /**
   * Arithmetic multiply operator
   * @param[in] other multiplicand
   */
  PaillierEncryptedNumber operator*(const BigNumber& other) const;

  /**
   * Apply obfuscator for ciphertext, obfuscated needed only when the result is
   * shared to untrusted parties
   */
  void apply_obfuscator() {
    b_isObfuscator = true;
    BigNumber obfuscator[8];
    m_pubkey->apply_obfuscator(obfuscator);

    BigNumber sq = m_pubkey->getNSQ();
    for (int i = 0; i < m_available; i++)
      m_bn[i] = sq.ModMul(m_bn[i], obfuscator[i]);
  }

  /**
   * Get output of Arithmetic operators
   * @param[in] idx index of output array
   */
  BigNumber getBN(size_t idx = 0) const {
    if (m_available == 1 && idx > 0)
      throw std::out_of_range("PaillierEncryptedNumber only has 1 BigNumber");

    return m_bn[idx];
  }

  /**
   * Get public key for python wrapper
   */
  PaillierPublicKey getPK() const { return *m_pubkey; }

  /**
   * Rotate PaillierEncryptedNumber
   */
  PaillierEncryptedNumber rotate(int shift);

  /**
   * Get array output of Arithmetic operators
   * @param[out] bn output array
   */
  void getArrayBN(BigNumber bn[8]) const { std::copy_n(m_bn, 8, bn); }

  /**
   * Element in PaillierEncryptedNumber is single
   */
  bool isSingle() const { return m_available == 1; }

  /**
   * Elements in PaillierEncryptedNumber
   */
  size_t getLength() const { return m_length; }

  const void* addr = static_cast<const void*>(this);

 private:
  bool b_isObfuscator;
  int m_available;
  PaillierPublicKey* m_pubkey;
  size_t m_length;
  BigNumber m_bn[8];

  BigNumber raw_add(const BigNumber& a, const BigNumber& b);
  BigNumber raw_mul(const BigNumber& a, const BigNumber& b);
  void raw_mul(BigNumber res[8], BigNumber a[8], BigNumber b[8]);
};

#endif  // IPCL_INCLUDE_IPCL_PAILLIER_OPS_HPP_
