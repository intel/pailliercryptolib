// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/ciphertext.hpp"

#include <algorithm>

#include "ipcl/mod_exp.hpp"

namespace ipcl {
CipherText::CipherText(const PublicKey& pk, const uint32_t& n)
    : BaseText(n), m_pk(std::make_shared<PublicKey>(pk)) {}

CipherText::CipherText(const PublicKey& pk, const std::vector<uint32_t>& n_v)
    : BaseText(n_v), m_pk(std::make_shared<PublicKey>(pk)) {}

CipherText::CipherText(const PublicKey& pk, const BigNumber& bn)
    : BaseText(bn), m_pk(std::make_shared<PublicKey>(pk)) {}

CipherText::CipherText(const PublicKey& pk, const std::vector<BigNumber>& bn_v)
    : BaseText(bn_v), m_pk(std::make_shared<PublicKey>(pk)) {}

CipherText::CipherText(const CipherText& ct) : BaseText(ct) {
  this->m_pk = ct.m_pk;
}

CipherText& CipherText::operator=(const CipherText& other) {
  BaseText::operator=(other);
  this->m_pk = other.m_pk;

  return *this;
}

// CT+CT
CipherText CipherText::operator+(const CipherText& other) const {
  std::size_t b_size = other.getSize();
  ERROR_CHECK(this->m_size == b_size || b_size == 1,
              "CT + CT error: Size mismatch!");
  ERROR_CHECK(*(m_pk->getN()) == *(other.m_pk->getN()),
              "CT + CT error: 2 different public keys detected!");

  const auto& a = *this;
  const auto& b = other;

  if (m_size == 1) {
    BigNumber sum = a.raw_add(a.m_texts.front(), b.getTexts().front());
    return CipherText(*m_pk, sum);
  } else {
    std::vector<BigNumber> sum(m_size);

    if (b_size == 1) {
// add vector by scalar
#ifdef IPCL_USE_OMP
      int omp_remaining_threads = OMPUtilities::MaxThreads;
#pragma omp parallel for num_threads( \
    OMPUtilities::assignOMPThreads(omp_remaining_threads, m_size))
#endif  // IPCL_USE_OMP
      for (std::size_t i = 0; i < m_size; i++)
        sum[i] = a.raw_add(a.m_texts[i], b.m_texts[0]);
    } else {
// add vector by vector
#ifdef IPCL_USE_OMP
      int omp_remaining_threads = OMPUtilities::MaxThreads;
#pragma omp parallel for num_threads( \
    OMPUtilities::assignOMPThreads(omp_remaining_threads, m_size))
#endif  // IPCL_USE_OMP
      for (std::size_t i = 0; i < m_size; i++)
        sum[i] = a.raw_add(a.m_texts[i], b.m_texts[i]);
    }
    return CipherText(*m_pk, sum);
  }
}

// CT + PT
CipherText CipherText::operator+(const PlainText& other) const {
  // convert PT to CT
  CipherText b = this->m_pk->encrypt(other, false);
  // calculate CT + CT
  return this->operator+(b);
}

// CT * PT
CipherText CipherText::operator*(const PlainText& other) const {
  std::size_t b_size = other.getSize();
  ERROR_CHECK(this->m_size == b_size || b_size == 1,
              "CT * PT error: Size mismatch!");

  const auto& a = *this;
  const auto& b = other;

  if (m_size == 1) {
    BigNumber product = a.raw_mul(a.m_texts.front(), b.getTexts().front());
    return CipherText(*m_pk, product);
  } else {
    std::vector<BigNumber> product;
    if (b_size == 1) {
      // multiply vector by scalar
      std::vector<BigNumber> b_v(a.m_size, b.getElement(0));
      product = a.raw_mul(a.m_texts, b_v);
    } else {
      // multiply vector by vector
      product = a.raw_mul(a.m_texts, b.getTexts());
    }
    return CipherText(*m_pk, product);
  }
}

CipherText CipherText::getCipherText(const size_t& idx) const {
  ERROR_CHECK((idx >= 0) && (idx < m_size),
              "CipherText::getCipherText index is out of range");

  return CipherText(*m_pk, m_texts[idx]);
}

std::shared_ptr<PublicKey> CipherText::getPubKey() const { return m_pk; }

CipherText CipherText::rotate(int shift) const {
  ERROR_CHECK(m_size != 1, "rotate: Cannot rotate single CipherText");
  ERROR_CHECK(shift >= (-1) * static_cast<int>(m_size) && shift <= m_size,
              "rotate: Cannot shift more than the test size");

  if (shift == 0 || shift == m_size || shift == (-1) * static_cast<int>(m_size))
    return CipherText(*m_pk, m_texts);

  if (shift > 0)
    shift = m_size - shift;
  else
    shift = -shift;

  std::vector<BigNumber> new_bn = getTexts();
  std::rotate(std::begin(new_bn), std::begin(new_bn) + shift, std::end(new_bn));
  return CipherText(*m_pk, new_bn);
}

BigNumber CipherText::raw_add(const BigNumber& a, const BigNumber& b) const {
  // Hold a copy of nsquare for multi-threaded
  // The BigNumber % operator is not thread safe
  // const BigNumber& sq = *(m_pk->getNSQ());
  const BigNumber sq = *(m_pk->getNSQ());
  return a * b % sq;
}

BigNumber CipherText::raw_mul(const BigNumber& a, const BigNumber& b) const {
  const BigNumber& sq = *(m_pk->getNSQ());
  return modExp(a, b, sq);
}

std::vector<BigNumber> CipherText::raw_mul(
    const std::vector<BigNumber>& a, const std::vector<BigNumber>& b) const {
  std::size_t v_size = a.size();
  std::vector<BigNumber> sq(v_size, *(m_pk->getNSQ()));

  // If hybrid OPTIMAL mode is used, use a special ratio
  if (isHybridOptimal()) {
    float qat_ratio = (v_size <= IPCL_WORKLOAD_SIZE_THRESHOLD)
                          ? IPCL_HYBRID_MODEXP_RATIO_FULL
                          : IPCL_HYBRID_MODEXP_RATIO_MULTIPLY;
    setHybridRatio(qat_ratio, false);
  }

  return modExp(a, b, sq);
}

}  // namespace ipcl
