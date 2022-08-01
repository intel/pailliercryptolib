// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_BASE_TEXT_HPP_
#define IPCL_INCLUDE_IPCL_BASE_TEXT_HPP_

#include <string>
#include <vector>

#include "ipcl/bignum.h"

namespace ipcl {

class BaseText {
 public:
  BaseText() = default;
  ~BaseText() = default;

  /**
   * BaseText constructors
   */
  explicit BaseText(const uint32_t& n);
  explicit BaseText(const std::vector<uint32_t>& n_v);
  explicit BaseText(const BigNumber& bn);
  explicit BaseText(const std::vector<BigNumber>& bn_v);

  /**
   * BaseText copy constructor
   */
  BaseText(const BaseText& bt);

  /**
   * BaseText assignment constructor
   */
  BaseText& operator=(const BaseText& other);

  /**
   * Overloading [] operator to access BigNumber elements
   */
  BigNumber& operator[](const std::size_t idx);

  /**
   * Insert a big number before pos
   * @param[in] pos Position in m_texts
   * @bn[in] Big number need to be inserted
   */
  void insert(const std::size_t pos, BigNumber& bn);

  /**
   * Clear all big number element
   */
  void clear();

  /**
   * Remove big number element for pos to pos+length.
   * @param[in] pos Position in m_texts
   * @length[in] Length need to be removed(default value is 1)
   */
  void remove(const std::size_t pos, const std::size_t length = 1);

  /**
   * Gets the specified BigNumber element in m_text
   * @param[in] idx Element index
   * return Element in m_text of type BigNumber
   */
  BigNumber getElement(const std::size_t& idx) const;

  /**
   * Gets the specified BigNumber vector form
   * @param[in] idx Element index
   * @return Element vector form
   */
  std::vector<uint32_t> getElementVec(const std::size_t& idx) const;

  /**
   * Gets the specified BigNumber hex string form
   * @param[in] idx Element index
   * @return Element hex string form
   */
  std::string getElementHex(const std::size_t& idx) const;

  /**
   * Gets a chunk of BigNumber element in m_text
   * @param[in] start Start position
   * @param[in] size The number of element
   * return A chunk of BigNumber element
   */
  std::vector<BigNumber> getChunk(const std::size_t& start,
                                  const std::size_t& size) const;

  /**
   * Gets the BigNumber container
   */
  std::vector<BigNumber> getTexts() const;

  /**
   * Gets the size of the BigNumber container
   */
  std::size_t getSize() const;

  const void* addr = static_cast<const void*>(this);

 protected:
  std::vector<BigNumber> m_texts;  ///< Container used to store BigNumber
  std::size_t m_size;              ///< Container size
};

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_BASE_TEXT_HPP_
