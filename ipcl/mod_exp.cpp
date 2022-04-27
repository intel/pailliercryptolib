// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/mod_exp.hpp"

#include <crypto_mb/exp.h>

#include <algorithm>
#include <cstring>
#include <iostream>

#include "ipcl/util.hpp"

namespace ipcl {

static std::vector<BigNumber> ippMBModExp(const std::vector<BigNumber>& base,
                                          const std::vector<BigNumber>& pow,
                                          const std::vector<BigNumber>& m) {
  VEC_SIZE_CHECK(base);
  VEC_SIZE_CHECK(pow);
  VEC_SIZE_CHECK(m);

  mbx_status st = MBX_STATUS_OK;

  int bits = m.front().BitSize();
  int dwords = BITSIZE_DWORD(bits);
  int bufferLen = mbx_exp_BufferSize(bits);
  auto buffer = std::vector<Ipp8u>(bufferLen);

  std::vector<int64u*> out_x(IPCL_CRYPTO_MB_SIZE);
  std::vector<int64u*> b_array(IPCL_CRYPTO_MB_SIZE);
  std::vector<int64u*> p_array(IPCL_CRYPTO_MB_SIZE);

  int mem_pool_size = IPCL_CRYPTO_MB_SIZE * dwords;
  std::vector<int64u> out_mem_pool(mem_pool_size);
  std::vector<int64u> base_mem_pool(mem_pool_size);
  std::vector<int64u> pow_mem_pool(mem_pool_size);

  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    out_x[i] = &out_mem_pool[i * dwords];
    b_array[i] = &base_mem_pool[i * dwords];
    p_array[i] = &pow_mem_pool[i * dwords];
  }

  /*
   * These two intermediate variables pow_b & pow_p are necessary
   * because if they are not used, the length returned from ippsRef_BN
   * will be inconsistent with the length allocated by b_array/p_array,
   * resulting in data errors.
   */
  std::vector<Ipp32u*> pow_b(IPCL_CRYPTO_MB_SIZE);
  std::vector<Ipp32u*> pow_p(IPCL_CRYPTO_MB_SIZE);
  std::vector<Ipp32u*> pow_nsquare(IPCL_CRYPTO_MB_SIZE);
  std::vector<int> b_size_v(IPCL_CRYPTO_MB_SIZE);
  std::vector<int> p_size_v(IPCL_CRYPTO_MB_SIZE);
  std::vector<int> n_size_v(IPCL_CRYPTO_MB_SIZE);

  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    ippsRef_BN(nullptr, &b_size_v[i], reinterpret_cast<Ipp32u**>(&pow_b[i]),
               base[i]);
    ippsRef_BN(nullptr, &p_size_v[i], reinterpret_cast<Ipp32u**>(&pow_p[i]),
               pow[i]);
    ippsRef_BN(nullptr, &n_size_v[i], &pow_nsquare[i], m[i]);

    memcpy(b_array[i], pow_b[i], BITSIZE_WORD(b_size_v[i]) * 4);
    memcpy(p_array[i], pow_p[i], BITSIZE_WORD(p_size_v[i]) * 4);
  }

  // Find the biggest size of module and exp
  int nsqBitLen = *std::max_element(n_size_v.begin(), n_size_v.end());
  int expBitLen = *std::max_element(p_size_v.begin(), p_size_v.end());

  // If actual sizes of modules are different,
  // set the mod_bits parameter equal to maximum size of the actual module in
  // bit size and extend all the modules with zero bits to the mod_bits value.
  // The same is applicable for the exp_bits parameter and actual exponents.
  st = mbx_exp_mb8(out_x.data(), b_array.data(), p_array.data(), expBitLen,
                   reinterpret_cast<Ipp64u**>(pow_nsquare.data()), nsqBitLen,
                   reinterpret_cast<Ipp8u*>(buffer.data()), bufferLen);

  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    ERROR_CHECK(MBX_STATUS_OK == MBX_GET_STS(st, i),
                std::string("ippMultiBuffExp: error multi buffered exp "
                            "modules, error code = ") +
                    std::to_string(MBX_GET_STS(st, i)));
  }

  // It is important to hold a copy of nsquare for thread-safe purpose
  std::vector<BigNumber> res(IPCL_CRYPTO_MB_SIZE, m.front());

  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    res[i].Set(reinterpret_cast<Ipp32u*>(out_x[i]), BITSIZE_WORD(nsqBitLen),
               IppsBigNumPOS);
  }
  return res;
}

static BigNumber ippSBModExp(const BigNumber& base, const BigNumber& pow,
                             const BigNumber& m) {
  IppStatus stat = ippStsNoErr;
  // It is important to declare res * bform bit length refer to ipp-crypto spec:
  // R should not be less than the data length of the modulus m
  BigNumber res(m);

  int bnBitLen;
  Ipp32u* pow_m;
  ippsRef_BN(nullptr, &bnBitLen, &pow_m, BN(m));
  int nlen = BITSIZE_WORD(bnBitLen);

  int size;
  // define and initialize Montgomery Engine over Modulus N
  stat = ippsMontGetSize(IppsBinaryMethod, nlen, &size);
  ERROR_CHECK(stat == ippStsNoErr,
              "ippMontExp: get the size of IppsMontState context error.");

  auto pMont = std::vector<Ipp8u>(size);

  stat = ippsMontInit(IppsBinaryMethod, nlen,
                      reinterpret_cast<IppsMontState*>(pMont.data()));
  ERROR_CHECK(stat == ippStsNoErr, "ippMontExp: init Mont context error.");

  stat =
      ippsMontSet(pow_m, nlen, reinterpret_cast<IppsMontState*>(pMont.data()));
  ERROR_CHECK(stat == ippStsNoErr, "ippMontExp: set Mont input error.");

  // encode base into Montgomery form
  BigNumber bform(m);
  stat = ippsMontForm(BN(base), reinterpret_cast<IppsMontState*>(pMont.data()),
                      BN(bform));
  ERROR_CHECK(stat == ippStsNoErr,
              "ippMontExp: convert big number into Mont form error.");

  // compute R = base^pow mod N
  stat = ippsMontExp(BN(bform), BN(pow),
                     reinterpret_cast<IppsMontState*>(pMont.data()), BN(res));
  ERROR_CHECK(stat == ippStsNoErr,
              std::string("ippsMontExp: error code = ") + std::to_string(stat));

  BigNumber one(1);
  // R = MontMul(R,1)
  stat = ippsMontMul(BN(res), BN(one),
                     reinterpret_cast<IppsMontState*>(pMont.data()), BN(res));

  ERROR_CHECK(stat == ippStsNoErr,
              std::string("ippsMontMul: error code = ") + std::to_string(stat));

  return res;
}

std::vector<BigNumber> ippModExp(const std::vector<BigNumber>& base,
                                 const std::vector<BigNumber>& pow,
                                 const std::vector<BigNumber>& m) {
  std::size_t v_size = base.size();
  std::vector<BigNumber> res(v_size);

#ifdef IPCL_CRYPTO_MB_MOD_EXP

  // If there is only 1 big number, we don't need to use MBModExp
  if (v_size == 1) {
    res[0] = ippSBModExp(base[0], pow[0], m[0]);
    return res;
  }

  std::size_t offset = 0;
  std::size_t num_chunk = v_size / IPCL_CRYPTO_MB_SIZE;
  std::size_t remainder = v_size % IPCL_CRYPTO_MB_SIZE;

#ifdef IPCL_USE_OMP
#pragma omp parallel for
#endif  // IPCL_USE_OMP
  for (std::size_t i = 0; i < num_chunk; i++) {
    auto base_start = base.begin() + i * IPCL_CRYPTO_MB_SIZE;
    auto base_end = base_start + IPCL_CRYPTO_MB_SIZE;

    auto pow_start = pow.begin() + i * IPCL_CRYPTO_MB_SIZE;
    auto pow_end = pow_start + IPCL_CRYPTO_MB_SIZE;

    auto m_start = m.begin() + i * IPCL_CRYPTO_MB_SIZE;
    auto m_end = m_start + IPCL_CRYPTO_MB_SIZE;

    auto base_chunk = std::vector<BigNumber>(base_start, base_end);
    auto pow_chunk = std::vector<BigNumber>(pow_start, pow_end);
    auto m_chunk = std::vector<BigNumber>(m_start, m_end);

    auto tmp = ippMBModExp(base_chunk, pow_chunk, m_chunk);
    std::copy(tmp.begin(), tmp.end(), res.begin() + i * IPCL_CRYPTO_MB_SIZE);
  }

  offset = num_chunk * IPCL_CRYPTO_MB_SIZE;
  // If only 1 big number left, we don't need to make padding
  if (remainder == 1)
    res[offset] = ippSBModExp(base[offset], pow[offset], m[offset]);

  // If the 1 < remainder < IPCL_CRYPTO_MB_SIZE, we need to make padding
  if (remainder > 1) {
    auto base_start = base.begin() + offset;
    auto base_end = base_start + remainder;

    auto pow_start = pow.begin() + offset;
    auto pow_end = pow_start + remainder;

    auto m_start = m.begin() + offset;
    auto m_end = m_start + remainder;

    std::vector<BigNumber> base_chunk(IPCL_CRYPTO_MB_SIZE, 0);
    std::vector<BigNumber> pow_chunk(IPCL_CRYPTO_MB_SIZE, 0);
    std::vector<BigNumber> m_chunk(IPCL_CRYPTO_MB_SIZE, 0);

    std::copy(base_start, base_end, base_chunk.begin());
    std::copy(pow_start, pow_end, pow_chunk.begin());
    std::copy(m_start, m_end, m_chunk.begin());

    auto tmp = ippMBModExp(base_chunk, pow_chunk, m_chunk);
    std::copy(tmp.begin(), tmp.begin() + remainder, res.begin() + offset);
  }

  return res;

#else

#ifdef IPCL_USE_OMP
#pragma omp parallel for
#endif  // IPCL_USE_OMP
  for (int i = 0; i < v_size; i++) res[i] = ippSBModExp(base[i], pow[i], m[i]);
  return res;

#endif  // IPCL_CRYPTO_MB_MOD_EXP
}

BigNumber ippModExp(const BigNumber& base, const BigNumber& pow,
                    const BigNumber& m) {
  return ippSBModExp(base, pow, m);
}

}  // namespace ipcl
