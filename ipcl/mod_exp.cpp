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
                                          const std::vector<BigNumber>& exp,
                                          const std::vector<BigNumber>& mod) {
  std::size_t real_v_size = base.size();
  std::size_t pow_v_size = exp.size();
  std::size_t mod_v_size = mod.size();
  ERROR_CHECK((real_v_size == pow_v_size) && (pow_v_size == mod_v_size) &&
                  (real_v_size <= IPCL_CRYPTO_MB_SIZE),
              "ippMBModExp: input vector size error");

  mbx_status st = MBX_STATUS_OK;

  /*
   * These two intermediate variables base_data & exp_data are necessary
   * because if they are not used, the length returned from ippsRef_BN
   * will be inconsistent with the length allocated by base_pa/exp_pa,
   * resulting in data errors.
   */
  std::vector<Ipp32u*> base_data(IPCL_CRYPTO_MB_SIZE);
  std::vector<Ipp32u*> exp_data(IPCL_CRYPTO_MB_SIZE);
  std::vector<Ipp32u*> mod_data(IPCL_CRYPTO_MB_SIZE);
  std::vector<int> base_bits_v(IPCL_CRYPTO_MB_SIZE);
  std::vector<int> exp_bits_v(IPCL_CRYPTO_MB_SIZE);
  std::vector<int> mod_bits_v(IPCL_CRYPTO_MB_SIZE);

  for (int i = 0; i < real_v_size; i++) {
    ippsRef_BN(nullptr, &base_bits_v[i],
               reinterpret_cast<Ipp32u**>(&base_data[i]), base[i]);
    ippsRef_BN(nullptr, &exp_bits_v[i],
               reinterpret_cast<Ipp32u**>(&exp_data[i]), exp[i]);
    ippsRef_BN(nullptr, &mod_bits_v[i], &mod_data[i], mod[i]);
  }

  // Find the longest size of module and power
  auto longest_mod_it = std::max_element(mod_bits_v.begin(), mod_bits_v.end());
  auto longest_mod_idx = std::distance(mod_bits_v.begin(), longest_mod_it);
  BigNumber longest_mod = mod[longest_mod_idx];
  int mod_bits = *longest_mod_it;
  int exp_bits = *std::max_element(exp_bits_v.begin(), exp_bits_v.end());

  std::vector<int64u*> out_pa(IPCL_CRYPTO_MB_SIZE);
  std::vector<int64u*> base_pa(IPCL_CRYPTO_MB_SIZE);
  std::vector<int64u*> exp_pa(IPCL_CRYPTO_MB_SIZE);

  int mod_dwords = BITSIZE_DWORD(mod_bits);
  int num_buff = IPCL_CRYPTO_MB_SIZE * mod_dwords;
  std::vector<int64u> out_buff(num_buff);
  std::vector<int64u> base_buff(num_buff);
  std::vector<int64u> exp_buff(num_buff);

  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    auto idx = i * mod_dwords;
    out_pa[i] = &out_buff[idx];
    base_pa[i] = &base_buff[idx];
    exp_pa[i] = &exp_buff[idx];
  }

  for (int i = 0; i < real_v_size; i++) {
    memcpy(base_pa[i], base_data[i], BITSIZE_WORD(base_bits_v[i]) * 4);
    memcpy(exp_pa[i], exp_data[i], BITSIZE_WORD(exp_bits_v[i]) * 4);
  }

  int work_buff_size = mbx_exp_BufferSize(mod_bits);
  auto work_buff = std::vector<Ipp8u>(work_buff_size);
  // If actual sizes of modules are different,
  // set the mod_bits parameter equal to maximum size of the actual module in
  // bit size and extend all the modules with zero bits to the mod_bits value.
  // The same is applicable for the exp_bits parameter and actual exponents.
  st = mbx_exp_mb8(out_pa.data(), base_pa.data(), exp_pa.data(), exp_bits,
                   reinterpret_cast<Ipp64u**>(mod_data.data()), mod_bits,
                   reinterpret_cast<Ipp8u*>(work_buff.data()), work_buff_size);

  for (int i = 0; i < real_v_size; i++) {
    ERROR_CHECK(MBX_STATUS_OK == MBX_GET_STS(st, i),
                std::string("ippMultiBuffExp: error multi buffered exp "
                            "modules, error code = ") +
                    std::to_string(MBX_GET_STS(st, i)));
  }

  // Init res with longest mod value to ensure each
  // Big number has enough space.
  std::vector<BigNumber> res(real_v_size, longest_mod);
  for (int i = 0; i < real_v_size; i++) {
    res[i].Set(reinterpret_cast<Ipp32u*>(out_pa[i]), BITSIZE_WORD(mod_bits),
               IppsBigNumPOS);
  }
  return res;
}

static BigNumber ippSBModExp(const BigNumber& base, const BigNumber& exp,
                             const BigNumber& mod) {
  IppStatus stat = ippStsNoErr;
  // It is important to declare res * bform bit length refer to ipp-crypto spec:
  // R should not be less than the data length of the modulus m
  BigNumber res(mod);

  int mod_bits;
  Ipp32u* mod_data;
  ippsRef_BN(nullptr, &mod_bits, &mod_data, BN(mod));
  int mod_words = BITSIZE_WORD(mod_bits);

  int size;
  // define and initialize Montgomery Engine over Modulus N
  stat = ippsMontGetSize(IppsBinaryMethod, mod_words, &size);
  ERROR_CHECK(stat == ippStsNoErr,
              "ippMontExp: get the size of IppsMontState context error.");

  auto pMont = std::vector<Ipp8u>(size);

  stat = ippsMontInit(IppsBinaryMethod, mod_words,
                      reinterpret_cast<IppsMontState*>(pMont.data()));
  ERROR_CHECK(stat == ippStsNoErr, "ippMontExp: init Mont context error.");

  stat = ippsMontSet(mod_data, mod_words,
                     reinterpret_cast<IppsMontState*>(pMont.data()));
  ERROR_CHECK(stat == ippStsNoErr, "ippMontExp: set Mont input error.");

  // encode base into Montgomery form
  BigNumber bform(mod);
  stat = ippsMontForm(BN(base), reinterpret_cast<IppsMontState*>(pMont.data()),
                      BN(bform));
  ERROR_CHECK(stat == ippStsNoErr,
              "ippMontExp: convert big number into Mont form error.");

  // compute R = base^pow mod N
  stat = ippsMontExp(BN(bform), BN(exp),
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
                                 const std::vector<BigNumber>& exp,
                                 const std::vector<BigNumber>& mod) {
  std::size_t v_size = base.size();
  std::vector<BigNumber> res(v_size);

#ifdef IPCL_RUNTIME_MOD_EXP

  // If there is only 1 big number, we don't need to use MBModExp
  if (v_size == 1) {
    res[0] = ippSBModExp(base[0], exp[0], mod[0]);
    return res;
  }

  if (has_avx512ifma) {
    std::size_t remainder = v_size % IPCL_CRYPTO_MB_SIZE;
    std::size_t num_chunk =
        (v_size + IPCL_CRYPTO_MB_SIZE - 1) / IPCL_CRYPTO_MB_SIZE;
#ifdef IPCL_USE_OMP
    int omp_remaining_threads = OMPUtilities::MaxThreads;
#pragma omp parallel for num_threads( \
    OMPUtilities::assignOMPThreads(omp_remaining_threads, num_chunk))
#endif  // IPCL_USE_OMP
    for (std::size_t i = 0; i < num_chunk; i++) {
      std::size_t chunk_size = IPCL_CRYPTO_MB_SIZE;
      if ((i == (num_chunk - 1)) && (remainder > 0)) chunk_size = remainder;

      std::size_t chunk_offset = i * IPCL_CRYPTO_MB_SIZE;

      auto base_start = base.begin() + chunk_offset;
      auto base_end = base_start + chunk_size;

      auto exp_start = exp.begin() + chunk_offset;
      auto exp_end = exp_start + chunk_size;

      auto mod_start = mod.begin() + chunk_offset;
      auto mod_end = mod_start + chunk_size;

      auto base_chunk = std::vector<BigNumber>(base_start, base_end);
      auto exp_chunk = std::vector<BigNumber>(exp_start, exp_end);
      auto mod_chunk = std::vector<BigNumber>(mod_start, mod_end);

      auto tmp = ippMBModExp(base_chunk, exp_chunk, mod_chunk);
      std::copy(tmp.begin(), tmp.end(), res.begin() + chunk_offset);
    }

    return res;

  } else {
#ifdef IPCL_USE_OMP
    int omp_remaining_threads = OMPUtilities::MaxThreads;
#pragma omp parallel for num_threads( \
    OMPUtilities::assignOMPThreads(omp_remaining_threads, v_size))
#endif  // IPCL_USE_OMP
    for (int i = 0; i < v_size; i++)
      res[i] = ippSBModExp(base[i], exp[i], mod[i]);
    return res;
  }

#else

#ifdef IPCL_CRYPTO_MB_MOD_EXP

  // If there is only 1 big number, we don't need to use MBModExp
  if (v_size == 1) {
    res[0] = ippSBModExp(base[0], exp[0], mod[0]);
    return res;
  }

  std::size_t remainder = v_size % IPCL_CRYPTO_MB_SIZE;
  std::size_t num_chunk =
      (v_size + IPCL_CRYPTO_MB_SIZE - 1) / IPCL_CRYPTO_MB_SIZE;

#ifdef IPCL_USE_OMP
  int omp_remaining_threads = OMPUtilities::MaxThreads;
#pragma omp parallel for num_threads( \
    OMPUtilities::assignOMPThreads(omp_remaining_threads, num_chunk))
#endif  // IPCL_USE_OMP
  for (std::size_t i = 0; i < num_chunk; i++) {
    std::size_t chunk_size = IPCL_CRYPTO_MB_SIZE;
    if ((i == (num_chunk - 1)) && (remainder > 0)) chunk_size = remainder;

    std::size_t chunk_offset = i * IPCL_CRYPTO_MB_SIZE;

    auto base_start = base.begin() + chunk_offset;
    auto base_end = base_start + chunk_size;

    auto exp_start = exp.begin() + chunk_offset;
    auto exp_end = exp_start + chunk_size;

    auto mod_start = mod.begin() + chunk_offset;
    auto mod_end = mod_start + chunk_size;

    auto base_chunk = std::vector<BigNumber>(base_start, base_end);
    auto exp_chunk = std::vector<BigNumber>(exp_start, exp_end);
    auto mod_chunk = std::vector<BigNumber>(mod_start, mod_end);

    auto tmp = ippMBModExp(base_chunk, exp_chunk, mod_chunk);
    std::copy(tmp.begin(), tmp.end(), res.begin() + chunk_offset);
  }

  return res;

#else

#ifdef IPCL_USE_OMP
  int omp_remaining_threads = OMPUtilities::MaxThreads;
#pragma omp parallel for num_threads( \
    OMPUtilities::assignOMPThreads(omp_remaining_threads, v_size))
#endif  // IPCL_USE_OMP
  for (int i = 0; i < v_size; i++)
    res[i] = ippSBModExp(base[i], exp[i], mod[i]);
  return res;

#endif  // IPCL_CRYPTO_MB_MOD_EXP

#endif  // IPCL_RUNTIME_MOD_EXP
}

BigNumber ippModExp(const BigNumber& base, const BigNumber& exp,
                    const BigNumber& mod) {
  return ippSBModExp(base, exp, mod);
}

}  // namespace ipcl
