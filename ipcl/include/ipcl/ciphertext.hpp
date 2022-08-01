// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_CIPHERTEXT_HPP_
#define IPCL_INCLUDE_IPCL_CIPHERTEXT_HPP_

#include <vector>

#include "ipcl/plaintext.hpp"
#include "ipcl/pub_key.hpp"
#include "ipcl/util.hpp"

namespace ipcl {

class CipherText : public BaseText {
 public:
  CipherText() = default;
  ~CipherText() = default;

  /**
   * CipherText constructors
   */
  CipherText(const PublicKey* pub_key, const uint32_t& n);
  CipherText(const PublicKey* pub_key, const std::vector<uint32_t>& n_v);
  CipherText(const PublicKey* pub_key, const BigNumber& bn);
  CipherText(const PublicKey* pub_key, const std::vector<BigNumber>& bn_vec);

  /**
   * CipherText copy constructor
   */
  CipherText(const CipherText& ct);
  /**
   * CipherText assignment constructor
   */
  CipherText& operator=(const CipherText& other);

  // CT+CT
  CipherText operator+(const CipherText& other) const;
  // CT+PT
  CipherText operator+(const PlainText& other) const;
  // CT*PT
  CipherText operator*(const PlainText& other) const;

  /**
   * Get ciphertext of idx
   */
  CipherText getCipherText(const size_t& idx) const;

  /**
   * Get public key
   */
  const PublicKey* getPubKey() const;

  /**
   * Rotate CipherText
   * @param[in] shift rotate length
   */
  CipherText rotate(int shift) const;

 private:
  BigNumber raw_add(const BigNumber& a, const BigNumber& b) const;
  BigNumber raw_mul(const BigNumber& a, const BigNumber& b) const;
  std::vector<BigNumber> raw_mul(const std::vector<BigNumber>& a,
                                 const std::vector<BigNumber>& b) const;

  const PublicKey* m_pubkey;  ///< Public key used to encrypt big number
};

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_CIPHERTEXT_HPP_
