// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/plaintext.hpp"

#include <algorithm>

#include "ipcl/ciphertext.hpp"
#include "ipcl/util.hpp"

namespace ipcl {

PlainText::PlainText(const uint32_t& n) : BaseText(n) {}

PlainText::PlainText(const std::vector<uint32_t>& n_v) : BaseText(n_v) {}

PlainText::PlainText(const BigNumber& bn) : BaseText(bn) {}

PlainText::PlainText(const std::vector<BigNumber>& bn_v) : BaseText(bn_v) {}

PlainText::PlainText(const PlainText& pt) : BaseText(pt) {}

PlainText& PlainText::operator=(const PlainText& other) {
  BaseText::operator=(other);

  return *this;
}

CipherText PlainText::operator+(const CipherText& other) const {
  return other.operator+(*this);
}

CipherText PlainText::operator*(const CipherText& other) const {
  return other.operator*(*this);
}

PlainText::operator std::vector<uint32_t>() {
  ERROR_CHECK(m_size > 0,
              "PlainText: type conversion to uint32_t vector error");
  std::vector<uint32_t> v;
  m_texts[0].num2vec(v);

  return v;
}

PlainText::operator BigNumber() {
  ERROR_CHECK(m_size > 0, "PlainText: type conversion to BigNumber error");
  return m_texts[0];
}

PlainText::operator std::vector<BigNumber>() {
  ERROR_CHECK(m_size > 0,
              "PlainText: type conversion to BigNumber vector error");
  return m_texts;
}

PlainText PlainText::rotate(int shift) const {
  ERROR_CHECK(m_size != 1, "rotate: Cannot rotate single CipherText");
  ERROR_CHECK(shift >= -m_size && shift <= m_size,
              "rotate: Cannot shift more than the test size");

  if (shift == 0 || shift == m_size || shift == -m_size)
    return PlainText(m_texts);

  if (shift > 0)
    shift = m_size - shift;
  else
    shift = -shift;

  std::vector<BigNumber> new_bn = getTexts();
  std::rotate(std::begin(new_bn), std::begin(new_bn) + shift, std::end(new_bn));
  return PlainText(new_bn);
}

}  // namespace ipcl
