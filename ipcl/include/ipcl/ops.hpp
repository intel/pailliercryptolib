// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_OPS_HPP_
#define IPCL_INCLUDE_IPCL_OPS_HPP_

#include <vector>

#include "ipcl/pub_key.hpp"
#include "ipcl/util.hpp"

namespace ipcl {

class EncryptedNumber {
 public:
  /**
   * EncryptedNumber constructor
   * @param[in] pub_key paillier public key
   * @param[in] bn ciphertext encrypted by paillier public key
   */
  EncryptedNumber(const PublicKey* pub_key, const BigNumber& bn);

  /**
   * EncryptedNumber constructor
   * @param[in] pub_key paillier public key
   * @param[in] bn array of ciphertexts encrypted by paillier public key
   * @param[in] length size of array(default value is IPCL_CRYPTO_MB_SIZE)
   */
  EncryptedNumber(const PublicKey* pub_key, const std::vector<BigNumber>& bn,
                  size_t length = IPCL_CRYPTO_MB_SIZE);

  /**
   * EncryptedNumber constructor
   * @param[in] pub_key paillier public key
   * @param[in] scalar array of integer scalars
   * @param[in] length size of array(default value is IPCL_CRYPTO_MB_SIZE)
   */
  EncryptedNumber(const PublicKey* pub_key, const std::vector<uint32_t>& scalar,
                  size_t length = IPCL_CRYPTO_MB_SIZE);

  /**
   * Arithmetic addition operator
   * @param[in] bn augend
   */
  EncryptedNumber operator+(const EncryptedNumber& bn) const;

  /**
   * Arithmetic addition operator
   * @param[in] other augend
   */
  EncryptedNumber operator+(const BigNumber& other) const;

  /**
   * Arithmetic addition operator
   * @param[in] other array of augend
   */
  EncryptedNumber operator+(const std::vector<BigNumber>& other) const;

  /**
   * Arithmetic multiply operator
   * @param[in] bn multiplicand
   */
  EncryptedNumber operator*(const EncryptedNumber& bn) const;

  /**
   * Arithmetic multiply operator
   * @param[in] other multiplicand
   */
  EncryptedNumber operator*(const BigNumber& other) const;

  /**
   * Apply obfuscator for ciphertext, obfuscated needed only when the ciphertext
   * is exposed
   */
  void apply_obfuscator();

  /**
   * Return ciphertext
   * @param[in] idx index of ciphertext stored in EncryptedNumber
   * (default = 0)
   */
  BigNumber getBN(size_t idx = 0) const {
    ERROR_CHECK(m_available != 1 || idx <= 0,
                "getBN: EncryptedNumber only has 1 BigNumber");

    return m_bn[idx];
  }

  /**
   * Get public key
   */
  PublicKey getPK() const { return *m_pubkey; }

  /**
   * Rotate EncryptedNumber
   * @param[in] shift rotate length
   */
  EncryptedNumber rotate(int shift) const;

  /**
   * Return entire ciphertext array
   * @param[out] bn output array
   */
  std::vector<BigNumber> getArrayBN() const { return m_bn; }

  /**
   * Check if element in EncryptedNumber is single
   */
  bool isSingle() const { return m_available == 1; }

  /**
   * Get size of array in EncryptedNumber
   */
  size_t getLength() const { return m_length; }

  const void* addr = static_cast<const void*>(this);

 private:
  bool b_isObfuscator;
  int m_available;
  const PublicKey* m_pubkey;
  size_t m_length;
  std::vector<BigNumber> m_bn;

  BigNumber raw_add(const BigNumber& a, const BigNumber& b) const;
  BigNumber raw_mul(const BigNumber& a, const BigNumber& b) const;
  std::vector<BigNumber> raw_mul(const std::vector<BigNumber>& a,
                                 const std::vector<BigNumber>& b) const;
};

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_OPS_HPP_
