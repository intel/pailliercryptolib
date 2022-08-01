// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/base_text.hpp"

#include "ipcl/util.hpp"

namespace ipcl {

BaseText::BaseText(const uint32_t& n) : m_texts{BigNumber(n)}, m_size(1) {}

BaseText::BaseText(const std::vector<uint32_t>& n_v) {
  for (auto& n : n_v) {
    m_texts.push_back(BigNumber(n));
  }
  m_size = m_texts.size();
}

BaseText::BaseText(const BigNumber& bn) : m_texts{bn}, m_size(1) {}

BaseText::BaseText(const std::vector<BigNumber>& bn_v)
    : m_texts(bn_v), m_size(m_texts.size()) {}

BaseText::BaseText(const BaseText& bt) {
  this->m_texts = bt.getTexts();
  this->m_size = bt.getSize();
}

BaseText& BaseText::operator=(const BaseText& other) {
  if (this == &other) return *this;

  this->m_texts = other.m_texts;
  this->m_size = other.m_size;
  return *this;
}

BigNumber& BaseText::operator[](const std::size_t idx) {
  ERROR_CHECK(idx < m_size, "BaseText:operator[] index is out of range");

  return m_texts[idx];
}

void BaseText::insert(const std::size_t pos, BigNumber& bn) {
  ERROR_CHECK((pos >= 0) && (pos <= m_size),
              "BaseText: insert position is out of range");

  auto it = m_texts.begin() + pos;
  m_texts.insert(it, bn);
  m_size++;
}

void BaseText::clear() {
  m_texts.clear();
  m_size = 0;
}

void BaseText::remove(const std::size_t pos, const std::size_t length) {
  ERROR_CHECK((pos >= 0) && (pos + length < m_size),
              "BaseText: remove position is out of range");

  auto start = m_texts.begin() + pos;
  auto end = start + length;
  m_texts.erase(start, end);
  m_size = m_size - length;
}

BigNumber BaseText::getElement(const std::size_t& idx) const {
  ERROR_CHECK(idx < m_size, "BaseText: getElement index is out of range");

  return m_texts[idx];
}

std::vector<uint32_t> BaseText::getElementVec(const std::size_t& idx) const {
  ERROR_CHECK(idx < m_size, "BaseText: getElementVec index is out of range");

  std::vector<uint32_t> v;
  m_texts[idx].num2vec(v);

  return v;
}

std::string BaseText::getElementHex(const std::size_t& idx) const {
  ERROR_CHECK(idx < m_size, "BaseText: getElementHex index is out of range");
  std::string s;
  m_texts[idx].num2hex(s);

  return s;
}

std::vector<BigNumber> BaseText::getChunk(const std::size_t& start,
                                          const std::size_t& size) const {
  ERROR_CHECK((start >= 0) && ((start + size) <= m_size),
              "BaseText: getChunk parameter is incorrect");

  auto it_start = m_texts.begin() + start;
  auto it_end = it_start + size;
  auto v = std::vector<BigNumber>(it_start, it_end);

  return v;
}

std::vector<BigNumber> BaseText::getTexts() const { return m_texts; }

std::size_t BaseText::getSize() const { return m_size; }

}  // namespace ipcl
