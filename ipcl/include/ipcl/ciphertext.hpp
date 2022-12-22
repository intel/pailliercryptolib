// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_CIPHERTEXT_HPP_
#define IPCL_INCLUDE_IPCL_CIPHERTEXT_HPP_

#include <memory>
#include <vector>

#include "ipcl/plaintext.hpp"
#include "ipcl/pub_key.hpp"
#include "ipcl/utils/util.hpp"

namespace ipcl {

class CipherText : public BaseText {
 public:
  CipherText() = default;
  ~CipherText() = default;

  /**
   * CipherText constructors
   */
  CipherText(const PublicKey& pk, const uint32_t& n);
  CipherText(const PublicKey& pk, const std::vector<uint32_t>& n_v);
  CipherText(const PublicKey& pk, const BigNumber& bn);
  CipherText(const PublicKey& pk, const std::vector<BigNumber>& bn_vec);

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
  std::shared_ptr<PublicKey> getPubKey() const;

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

  std::shared_ptr<PublicKey> m_pk;  ///< Public key used to encrypt big number
};

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_CIPHERTEXT_HPP_
