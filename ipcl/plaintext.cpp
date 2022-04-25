// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/plaintext.hpp"

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

}  // namespace ipcl
