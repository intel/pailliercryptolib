// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_PUB_KEY_HPP_
#define IPCL_INCLUDE_IPCL_PUB_KEY_HPP_

#include <memory>
#include <utility>
#include <vector>

#include "ipcl/bignum.h"
#include "ipcl/plaintext.hpp"

namespace ipcl {

class CipherText;

class PublicKey {
 public:
  PublicKey() = default;
  ~PublicKey() = default;

  /**
   * PublicKey constructor
   * @param[in] n n of public key in paillier scheme
   * @param[in] bits bit length of public key(default value is 1024)
   * @param[in] enableDJN_ enables DJN scheme(default value is false)
   */
  explicit PublicKey(const BigNumber& n, int bits = 1024,
                     bool enableDJN_ = false);

  /**
   * PublicKey constructor
   * @param[in] n n of public key in paillier scheme
   * @param[in] bits bit length of public key(default value is 1024)
   * @param[in] enableDJN_ enables DJN scheme(default value is false)
   */

  explicit PublicKey(const Ipp32u n, int bits = 1024, bool enableDJN_ = false)
      : PublicKey(BigNumber(n), bits, enableDJN_) {}

  /**
   * DJN enabling function
   */
  void enableDJN();

  /**
   * Set DJN with given parameters
   * @param[in] hs hs value for DJN scheme
   * @param[in] randbit random bit for DJN scheme
   */
  void setDJN(const BigNumber& hs, int randbit);

  /**
   * Encrypt plaintext
   * @param[in] plaintext of type PlainText
   * @param[in] make_secure apply obfuscator(default value is true)
   * @return ciphertext of type CipherText
   */
  CipherText encrypt(const PlainText& plaintext, bool make_secure = true) const;

  /**
   * Get N of public key in paillier scheme
   */
  std::shared_ptr<BigNumber> getN() const { return m_n; }

  /**
   * Get NSQ of public key in paillier scheme
   */
  std::shared_ptr<BigNumber> getNSQ() const { return m_nsquare; }

  /**
   * Get G of public key in paillier scheme
   */
  std::shared_ptr<BigNumber> getG() const { return m_g; }

  /**
   * Get bits of key
   */
  int getBits() const { return m_bits; }

  /**
   * Get Dword of key
   */
  int getDwords() const { return m_dwords; }

  /**
   * Apply obfuscator for ciphertext
   * @param[out] obfuscator output of obfuscator with random value
   */
  void applyObfuscator(std::vector<BigNumber>& ciphertext) const;

  /**
   * Set the Random object for ISO/IEC 18033-6 compliance check
   * @param[in] r
   */
  void setRandom(const std::vector<BigNumber>& r);

  void setHS(const BigNumber& hs);

  /**
   * Check if using DJN scheme
   */
  bool isDJN() const { return m_enable_DJN; }

  /**
   * Get hs for DJN scheme
   */
  BigNumber getHS() const {
    if (m_enable_DJN) return m_hs;
    return BigNumber::Zero();
  }

  /**
   * Get randbits for DJN scheme
   */
  int getRandBits() const {
    if (m_enable_DJN) return m_randbits;
    return -1;
  }

  /**
   * Check whether pub key is initialized
   */
  bool isInitialized() { return m_isInitialized; }

  void create(const BigNumber& n, int bits, bool enableDJN_ = false);
  void create(const BigNumber& n, int bits, const BigNumber& hs, int randbits);

  const void* addr = static_cast<const void*>(this);

 private:
  friend class cereal::access;
  template <class Archive>
  void save(Archive& ar, const Ipp32u version) const {
    ar(::cereal::make_nvp("bits", m_bits));
    ar(::cereal::make_nvp("enable_DJN", m_enable_DJN));
    ar(::cereal::make_nvp("randbits", m_randbits));
    ar(::cereal::make_nvp("n", *m_n));
    ar(::cereal::make_nvp("hs", m_hs));
  }

  template <class Archive>
  void load(Archive& ar, const Ipp32u version) {
    bool enable_DJN;
    int bits, randbits;
    ar(::cereal::make_nvp("bits", bits));
    ar(::cereal::make_nvp("enable_DJN", enable_DJN));
    ar(::cereal::make_nvp("randbits", randbits));

    int bn_len = bits / 32;
    std::vector<Ipp32u> n_v(bn_len, 0);
    std::vector<Ipp32u> hs_v(bn_len * 2, 0);
    BigNumber n(n_v.data(), bn_len);
    BigNumber hs(hs_v.data(), bn_len * 2);

    ar(::cereal::make_nvp("n", n));
    ar(::cereal::make_nvp("hs", hs));

    if (enable_DJN)
      create(n, bits, hs, randbits);
    else
      create(n, bits);
  }

  bool m_isInitialized = false;
  std::shared_ptr<BigNumber> m_n;
  std::shared_ptr<BigNumber> m_g;
  std::shared_ptr<BigNumber> m_nsquare;
  int m_bits;
  int m_dwords;
  BigNumber m_hs;
  int m_randbits;
  bool m_enable_DJN;
  std::vector<BigNumber> m_r;
  bool m_testv;

  /**
   * Big number vector multi buffer encryption
   * @param[in] pt plaintext of BigNumber vector type
   * @param[in] make_secure apply obfuscator(default value is true)
   * @return ciphertext of BigNumber vector type
   */
  std::vector<BigNumber> raw_encrypt(const std::vector<BigNumber>& pt,
                                     bool make_secure = true) const;

  std::vector<BigNumber> getDJNObfuscator(std::size_t sz) const;

  std::vector<BigNumber> getNormalObfuscator(std::size_t sz) const;
};

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_PUB_KEY_HPP_
