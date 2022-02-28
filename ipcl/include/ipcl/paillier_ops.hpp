// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_PAILLIER_OPS_HPP_
#define IPCL_INCLUDE_IPCL_PAILLIER_OPS_HPP_

#include <algorithm>

#include "ipcl/paillier_pubkey.hpp"

class PaillierEncryptedNumber {
 public:
  /**
   * PaillierEncryptedNumber constructor
   * @param[in] pub_key paillier public key
   * @param[in] bn ciphertext encrypted by paillier public key
   */
  PaillierEncryptedNumber(PaillierPublicKey* pub_key, const BigNumber& bn);

  /**
   * PaillierEncryptedNumber constructor
   * @param[in] pub_key paillier public key
   * @param[in] bn array of ciphertexts encrypted by paillier public key
   * @param[in] length size of array
   */
  PaillierEncryptedNumber(PaillierPublicKey* pub_key, const BigNumber bn[8],
                          size_t length = 8);

  /**
   * PaillierEncryptedNumber constructor
   * @param[in] pub_key paillier public key
   * @param[in] scalar array of integer scalars
   * @param[in] length size of array
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
   * Apply obfuscator for ciphertext, obfuscated needed only when the ciphertext
   * is exposed
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
   * Return ciphertext
   * @param[in] idx index of ciphertext stored in PaillierEncryptedNumber
   */
  BigNumber getBN(size_t idx = 0) const {
    if (m_available == 1 && idx > 0)
      throw std::out_of_range("PaillierEncryptedNumber only has 1 BigNumber");

    return m_bn[idx];
  }

  /**
   * Get public key
   */
  PaillierPublicKey getPK() const { return *m_pubkey; }

  /**
   * Rotate PaillierEncryptedNumber
   * @param[in] shift rotate length
   */
  PaillierEncryptedNumber rotate(int shift);

  /**
   * Return entire ciphertext array
   * @param[out] bn output array
   */
  void getArrayBN(BigNumber bn[8]) const { std::copy_n(m_bn, 8, bn); }

  /**
   * Check if element in PaillierEncryptedNumber is single
   */
  bool isSingle() const { return m_available == 1; }

  /**
   * Get size of array in PaillierEncryptedNumber
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
