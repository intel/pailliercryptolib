// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_PAILLIER_OPS_HPP_
#define IPCL_INCLUDE_IPCL_PAILLIER_OPS_HPP_

#include <vector>

#include "ipcl/paillier_pubkey.hpp"
#include "ipcl/util.hpp"

namespace ipcl {

class PaillierEncryptedNumber {
 public:
  /**
   * PaillierEncryptedNumber constructor
   * @param[in] pub_key paillier public key
   * @param[in] bn ciphertext encrypted by paillier public key
   */
  PaillierEncryptedNumber(const PaillierPublicKey* pub_key,
                          const BigNumber& bn);

  /**
   * PaillierEncryptedNumber constructor
   * @param[in] pub_key paillier public key
   * @param[in] bn array of ciphertexts encrypted by paillier public key
   * @param[in] length size of array(default value is 8)
   */
  PaillierEncryptedNumber(const PaillierPublicKey* pub_key,
                          const std::vector<BigNumber>& bn, size_t length = 8);

  /**
   * PaillierEncryptedNumber constructor
   * @param[in] pub_key paillier public key
   * @param[in] scalar array of integer scalars
   * @param[in] length size of array(default value is 8)
   */
  PaillierEncryptedNumber(const PaillierPublicKey* pub_key,
                          const std::vector<uint32_t>& scalar,
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
  PaillierEncryptedNumber operator+(const std::vector<BigNumber>& other) const;

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
    std::vector<BigNumber> obfuscator(8);
    m_pubkey->apply_obfuscator(obfuscator);

    BigNumber sq = m_pubkey->getNSQ();
    for (int i = 0; i < m_available; i++)
      m_bn[i] = sq.ModMul(m_bn[i], obfuscator[i]);
  }

  /**
   * Return ciphertext
   * @param[in] idx index of ciphertext stored in PaillierEncryptedNumber
   * @param[in] idx index of output array(default value is 0)
   */
  BigNumber getBN(size_t idx = 0) const {
    ERROR_CHECK(m_available != 1 || idx <= 0,
                "getBN: PaillierEncryptedNumber only has 1 BigNumber");

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
  PaillierEncryptedNumber rotate(int shift) const;

  /**
   * Return entire ciphertext array
   * @param[out] bn output array
   */
  std::vector<BigNumber> getArrayBN() const { return m_bn; }

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
  const PaillierPublicKey* m_pubkey;
  size_t m_length;
  std::vector<BigNumber> m_bn;

  BigNumber raw_add(const BigNumber& a, const BigNumber& b) const;
  BigNumber raw_mul(const BigNumber& a, const BigNumber& b) const;
  std::vector<BigNumber> raw_mul(const std::vector<BigNumber>& a,
                                 const std::vector<BigNumber>& b) const;
};

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_PAILLIER_OPS_HPP_
