// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_PLAINTEXT_HPP_
#define IPCL_INCLUDE_IPCL_PLAINTEXT_HPP_

#include <vector>

#include "ipcl/base_text.hpp"

namespace ipcl {

class CipherText;
/**
 * This structure encapsulates types uint32_t,
 * uint32_t vector, BigNumber and BigNumber vector.
 */
class PlainText : public BaseText {
 public:
  PlainText() = default;
  ~PlainText() = default;

  /**
   * PlainText constructor
   * @param[in] n Reference to a uint32_t integer
   */
  explicit PlainText(const uint32_t& n);

  /**
   * PlainText constructor
   * @param[in] n_v Reference to a uint32_t vector
   */
  explicit PlainText(const std::vector<uint32_t>& n_v);

  /**
   * PlainText constructor
   * @param[in] bn Reference to a BigNumber
   */
  explicit PlainText(const BigNumber& bn);

  /**
   * PlainText constructor
   * @param[in] bn_v Reference to a BigNumber vector
   */
  explicit PlainText(const std::vector<BigNumber>& bn_v);

  /**
   * PlainText copy constructor
   */
  PlainText(const PlainText& pt);

  /**
   * PlainText assignment constructor
   */
  PlainText& operator=(const PlainText& other);

  /**
   * User define implicit type conversion
   * Convert 1st element to uint32_t vector.
   */
  operator std::vector<uint32_t>();

  /**
   * PT + CT
   */
  CipherText operator+(const CipherText& other) const;

  /**
   * PT * CT
   */
  CipherText operator*(const CipherText& other) const;

  /**
   * User define implicit type conversion
   * Convert 1st element to type BigNumber.
   */
  operator BigNumber();

  /**
   * User define implicit type conversion
   * Convert all element to type BigNumber.
   */
  operator std::vector<BigNumber>();

  /**
   * Rotate PlainText
   * @param[in] shift rotate length
   */
  PlainText rotate(int shift) const;
};

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_PLAINTEXT_HPP_
