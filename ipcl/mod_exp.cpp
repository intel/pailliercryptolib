// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/mod_exp.hpp"

#include <crypto_mb/exp.h>

#include <algorithm>
#include <cstring>
#include <iostream>

#ifdef IPCL_USE_QAT
#include <heqat/bnops.h>
#include <heqat/common.h>
#endif

#include "ipcl/util.hpp"

namespace ipcl {

#ifdef IPCL_USE_QAT

// Multiple input QAT ModExp interface to offload computation to QAT
static std::vector<BigNumber> heQatBnModExp(
    const std::vector<BigNumber>& base, const std::vector<BigNumber>& exponent,
    const std::vector<BigNumber>& modulus, unsigned int batch_size) {
  static unsigned int counter = 0;
  int nbits = modulus.front().BitSize();
  int length = BITSIZE_WORD(nbits) * 4;
  nbits = 8 * length;

  // Check if QAT Exec Env supports requested batch size
  unsigned int worksize = base.size();
  unsigned int nslices = worksize / batch_size;
  unsigned int residue = worksize % batch_size;

  if (0 == nslices) {
    nslices = 1;
    residue = 0;
    batch_size = worksize;
  }

  HE_QAT_STATUS status = HE_QAT_STATUS_FAIL;

  // TODO(fdiasmor): Try replace calloc by alloca to see impact on performance.
  unsigned char* bn_base_data_[batch_size];
  unsigned char* bn_exponent_data_[batch_size];
  unsigned char* bn_modulus_data_[batch_size];
  unsigned char* bn_remainder_data_[batch_size];

#if defined(IPCL_USE_QAT_LITE)
  // int base_len_[batch_size];
  std::vector<int> base_len_(batch_size, 0);
  // int exp_len_[batch_size];
  std::vector<int> exp_len_(batch_size, 0);
#endif

  // Pre-allocate memory used to batch input data
  for (int i = 0; i < batch_size; i++) {
#if !defined(IPCL_USE_QAT_LITE)
    bn_base_data_[i] = reinterpret_cast<unsigned char*>(
        malloc(length * sizeof(unsigned char)));
    bn_exponent_data_[i] = reinterpret_cast<unsigned char*>(
        malloc(length * sizeof(unsigned char)));
#endif

    bn_modulus_data_[i] = reinterpret_cast<unsigned char*>(
        malloc(length * sizeof(unsigned char)));
    bn_remainder_data_[i] = reinterpret_cast<unsigned char*>(
        malloc(length * sizeof(unsigned char)));

#if !defined(IPCL_USE_QAT_LITE)
    ERROR_CHECK(
        bn_base_data_[i] != nullptr && bn_exponent_data_[i] != nullptr &&
            bn_modulus_data_[i] != nullptr && bn_remainder_data_[i] != nullptr,
        "qatMultiBuffExp: alloc memory for error");
#else
    ERROR_CHECK(
        bn_modulus_data_[i] != nullptr && bn_remainder_data_[i] != nullptr,
        "qatMultiBuffExp: alloc memory for error");
#endif
  }  // End preparing input containers

  // Container to hold total number of outputs to be returned
  std::vector<BigNumber> remainder(worksize, 0);

  for (unsigned int j = 0; j < nslices; j++) {
    // Prepare batch of input data
    for (unsigned int i = 0; i < batch_size; i++) {
      bool ret = false;

#if !defined(IPCL_USE_QAT_LITE)
      memset(bn_base_data_[i], 0, length);
      memset(bn_exponent_data_[i], 0, length);
      memset(bn_modulus_data_[i], 0, length);
      memset(bn_remainder_data_[i], 0, length);

      ret =
          BigNumber::toBin(bn_base_data_[i], length, base[j * batch_size + i]);
      if (!ret) {
        printf("bn_base_data_: failed at bigNumberToBin()\n");
        exit(1);
      }

      ret = BigNumber::toBin(bn_exponent_data_[i], length,
                             exponent[j * batch_size + i]);
      if (!ret) {
        printf("bn_exponent_data_: failed at bigNumberToBin()\n");
        exit(1);
      }
#else
      base_len_[i] = 0;
      ret = BigNumber::toBin(&bn_base_data_[i], &base_len_[i],
                             base[j * batch_size + i]);
      if (!ret) {
        printf("bn_base_data_: failed at bigNumberToBin()\n");
        exit(1);
      }
      exp_len_[i] = 0;
      ret = BigNumber::toBin(&bn_exponent_data_[i], &exp_len_[i],
                             exponent[j * batch_size + i]);
      if (!ret) {
        printf("bn_exponent_data_: failed at bigNumberToBin()\n");
        exit(1);
      }
#endif

      ret = BigNumber::toBin(bn_modulus_data_[i], length,
                             modulus[j * batch_size + i]);
      if (!ret) {
        printf("bn_modulus_data_: failed at bigNumberToBin()\n");
        exit(1);
      }
    }  // End input setup

    // Process batch of input
    for (unsigned int i = 0; i < batch_size; i++) {
#if !defined(IPCL_USE_QAT_LITE)
      // Assumes all inputs and the output have the same length
      status =
          HE_QAT_bnModExp(bn_remainder_data_[i], bn_base_data_[i],
                          bn_exponent_data_[i], bn_modulus_data_[i], nbits);
#else
      // Base and exponent can be of variable length (for more or less)
      status = HE_QAT_bnModExp_lite(bn_remainder_data_[i], bn_base_data_[i],
                                    base_len_[i], bn_exponent_data_[i],
                                    exp_len_[i], bn_modulus_data_[i], nbits);
#endif
      if (HE_QAT_STATUS_SUCCESS != status) {
        HE_QAT_PRINT_ERR("\nQAT bnModExp with BigNumber failed\n");
      }
    }
    getBnModExpRequest(batch_size);

    // Collect results and pack them into BigNumber
    for (unsigned int i = 0; i < batch_size; i++) {
      bool ret = BigNumber::fromBin(remainder[j * batch_size + i],
                                    bn_remainder_data_[i], length);
      if (!ret) {
        printf("bn_remainder_data_: failed at bignumbertobin()\n");
        exit(1);
      }
#if defined(IPCL_USE_QAT_LITE)
      free(bn_base_data_[i]);
      bn_base_data_[i] = NULL;
      free(bn_exponent_data_[i]);
      bn_exponent_data_[i] = NULL;
#endif
    }
  }  // Batch Process

  // Takes care of remaining
  if (residue) {
    for (unsigned int i = 0; i < residue; i++) {
#if !defined(IPCL_USE_QAT_LITE)
      memset(bn_base_data_[i], 0, length);
      memset(bn_exponent_data_[i], 0, length);
#endif
      memset(bn_modulus_data_[i], 0, length);
      memset(bn_remainder_data_[i], 0, length);
    }

    for (unsigned int i = 0; i < residue; i++) {
      bool ret = false;
#if !defined(IPCL_USE_QAT_LITE)
      ret = BigNumber::toBin(bn_base_data_[i], length,
                             base[nslices * batch_size + i]);
      if (!ret) {
        printf("bn_base_data_: failed at bigNumberToBin()\n");
        exit(1);
      }
      ret = BigNumber::toBin(bn_exponent_data_[i], length,
                             exponent[nslices * batch_size + i]);
      if (!ret) {
        printf("bn_exponent_data_: failed at bigNumberToBin()\n");
        exit(1);
      }
#else
      base_len_[i] = 0;
      ret = BigNumber::toBin(&bn_base_data_[i], &base_len_[i],
                             base[nslices * batch_size + i]);
      if (!ret) {
        printf("bn_base_data_: failed at bigNumberToBin()\n");
        exit(1);
      }
      exp_len_[i] = 0;
      ret = BigNumber::toBin(&bn_exponent_data_[i], &exp_len_[i],
                             exponent[nslices * batch_size + i]);
      if (!ret) {
        printf("bn_exponent_data_: failed at bigNumberToBin()\n");
        exit(1);
      }
#endif

      ret = BigNumber::toBin(bn_modulus_data_[i], length,
                             modulus[nslices * batch_size + i]);
      if (!ret) {
        printf("bn_modulus_data_: failed at bigNumberToBin()\n");
        exit(1);
      }
    }  //

    for (unsigned int i = 0; i < residue; i++) {
#if !defined(IPCL_USE_QAT_LITE)
      // Assumes all inputs and the output have the same length
      status =
          HE_QAT_bnModExp(bn_remainder_data_[i], bn_base_data_[i],
                          bn_exponent_data_[i], bn_modulus_data_[i], nbits);
#else
      // Base and exponent can be of variable length (for more or less)
      status = HE_QAT_bnModExp_lite(bn_remainder_data_[i], bn_base_data_[i],
                                    base_len_[i], bn_exponent_data_[i],
                                    exp_len_[i], bn_modulus_data_[i], nbits);
#endif
      if (HE_QAT_STATUS_SUCCESS != status) {
        HE_QAT_PRINT_ERR("\nQAT bnModExp with BigNumber failed\n");
      }
    }
    getBnModExpRequest(residue);

    // Collect results and pack them into BigNumber
    for (unsigned int i = 0; i < residue; i++) {
      unsigned char* bn_remainder_ = bn_remainder_data_[i];
      bool ret = BigNumber::fromBin(remainder[nslices * batch_size + i],
                                    bn_remainder_, length);
      if (!ret) {
        printf("residue bn_remainder_data_: failed at BigNumber::fromBin()\n");
        exit(1);
      }
#if defined(IPCL_USE_QAT_LITE)
      free(bn_base_data_[i]);
      bn_base_data_[i] = NULL;
      free(bn_exponent_data_[i]);
      bn_exponent_data_[i] = NULL;
#endif
    }
  }

  for (unsigned int i = 0; i < batch_size; i++) {
#if !defined(IPCL_USE_QAT_LITE)
    free(bn_base_data_[i]);
    bn_base_data_[i] = NULL;
    free(bn_exponent_data_[i]);
    bn_exponent_data_[i] = NULL;
#endif
    free(bn_modulus_data_[i]);
    bn_modulus_data_[i] = NULL;
  }

  for (unsigned int i = 0; i < batch_size; i++) {
    free(bn_remainder_data_[i]);
    bn_remainder_data_[i] = NULL;
  }

  return remainder;
}

// Multiple input QAT ModExp interface to offload computation to QAT
static std::vector<BigNumber> heQatBnModExp(
    const std::vector<BigNumber>& base, const std::vector<BigNumber>& exponent,
    const std::vector<BigNumber>& modulus) {
  static unsigned int counter = 0;
  int nbits = modulus.front().BitSize();
  int length = BITSIZE_WORD(nbits) * 4;
  nbits = 8 * length;

  HE_QAT_STATUS status = HE_QAT_STATUS_FAIL;

  // TODO(fdiasmor): Try replace calloc by alloca to see impact on performance.
  unsigned char* bn_base_data_[IPCL_CRYPTO_MB_SIZE];
  unsigned char* bn_exponent_data_[IPCL_CRYPTO_MB_SIZE];
  unsigned char* bn_modulus_data_[IPCL_CRYPTO_MB_SIZE];
  unsigned char* bn_remainder_data_[IPCL_CRYPTO_MB_SIZE];
  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    bn_base_data_[i] = reinterpret_cast<unsigned char*>(
        malloc(length * sizeof(unsigned char)));
    bn_exponent_data_[i] = reinterpret_cast<unsigned char*>(
        malloc(length * sizeof(unsigned char)));
    bn_modulus_data_[i] = reinterpret_cast<unsigned char*>(
        malloc(length * sizeof(unsigned char)));
    bn_remainder_data_[i] = reinterpret_cast<unsigned char*>(
        malloc(length * sizeof(unsigned char)));

    ERROR_CHECK(
        bn_base_data_[i] != nullptr && bn_exponent_data_[i] != nullptr &&
            bn_modulus_data_[i] != nullptr && bn_remainder_data_[i] != nullptr,
        "qatMultiBuffExp: alloc memory for error");

    memset(bn_base_data_[i], 0, length);
    memset(bn_exponent_data_[i], 0, length);
    memset(bn_modulus_data_[i], 0, length);
    memset(bn_remainder_data_[i], 0, length);
  }

  // TODO(fdiasmor): Define and use IPCL_QAT_BATCH_SIZE instead of
  // IPCL_CRYPTO_MB_SIZE.
  for (unsigned int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    bool ret = BigNumber::toBin(bn_base_data_[i], length, base[i]);
    if (!ret) {
      printf("bn_base_data_: failed at bigNumberToBin()\n");
      exit(1);
    }
    ret = BigNumber::toBin(bn_exponent_data_[i], length, exponent[i]);
    if (!ret) {
      printf("bn_exponent_data_: failed at bigNumberToBin()\n");
      exit(1);
    }
    ret = BigNumber::toBin(bn_modulus_data_[i], length, modulus[i]);
    if (!ret) {
      printf("bn_modulus_data_: failed at bigNumberToBin()\n");
      exit(1);
    }

    unsigned char* bn_base_ =
        reinterpret_cast<unsigned char*>(bn_base_data_[i]);
    unsigned char* bn_exponent_ =
        reinterpret_cast<unsigned char*>(bn_exponent_data_[i]);
    unsigned char* bn_modulus_ =
        reinterpret_cast<unsigned char*>(bn_modulus_data_[i]);
    unsigned char* bn_remainder_ =
        reinterpret_cast<unsigned char*>(bn_remainder_data_[i]);

    status = HE_QAT_bnModExp(bn_remainder_, bn_base_, bn_exponent_, bn_modulus_,
                             nbits);
    if (HE_QAT_STATUS_SUCCESS != status) {
      HE_QAT_PRINT_ERR("\nQAT bnModExp with BigNumber failed\n");
    }
  }
  getBnModExpRequest(IPCL_CRYPTO_MB_SIZE);

  std::vector<BigNumber> remainder(IPCL_CRYPTO_MB_SIZE, 0);
  // Collect results and pack them into BigNumber
  for (unsigned int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    unsigned char* bn_remainder_ = bn_remainder_data_[i];
    bool ret = BigNumber::fromBin(remainder[i], bn_remainder_, length);
    if (!ret) {
      printf("bn_remainder_data_: failed at bignumbertobin()\n");
      exit(1);
    }
  }

  for (unsigned int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    free(bn_base_data_[i]);
    bn_base_data_[i] = NULL;
    free(bn_exponent_data_[i]);
    bn_exponent_data_[i] = NULL;
    free(bn_modulus_data_[i]);
    bn_modulus_data_[i] = NULL;
  }

  for (unsigned int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    free(bn_remainder_data_[i]);
    bn_remainder_data_[i] = NULL;
  }

  return remainder;
}

// Single input QAT ModExp interface to offload computation to QAT
static BigNumber heQatBnModExp(const BigNumber& base, const BigNumber& exponent,
                               const BigNumber& modulus) {
  static unsigned int counter = 0;
  int nbits = modulus.BitSize();
  int length = BITSIZE_WORD(nbits) * 4;
  nbits = 8 * length;

  HE_QAT_STATUS status = HE_QAT_STATUS_FAIL;

  // TODO(fdiasmor): Try replace calloc by alloca to see impact on performance.
  unsigned char* bn_base_data_ = NULL;
  unsigned char* bn_exponent_data_ = NULL;
  unsigned char* bn_modulus_data_ = NULL;
  unsigned char* bn_remainder_data_ = NULL;

  bn_base_data_ =
      reinterpret_cast<unsigned char*>(malloc(length * sizeof(unsigned char)));
  bn_exponent_data_ =
      reinterpret_cast<unsigned char*>(malloc(length * sizeof(unsigned char)));
  bn_modulus_data_ =
      reinterpret_cast<unsigned char*>(malloc(length * sizeof(unsigned char)));
  bn_remainder_data_ =
      reinterpret_cast<unsigned char*>(malloc(length * sizeof(unsigned char)));

  ERROR_CHECK(bn_base_data_ != nullptr && bn_exponent_data_ != nullptr &&
                  bn_modulus_data_ != nullptr && bn_remainder_data_ != nullptr,
              "qatMultiBuffExp: alloc memory for error");

  memset(bn_base_data_, 0, length);
  memset(bn_exponent_data_, 0, length);
  memset(bn_modulus_data_, 0, length);
  memset(bn_remainder_data_, 0, length);

  bool ret = BigNumber::toBin(bn_base_data_, length, base);
  if (!ret) {
    printf("bn_base_data_: failed at bignumbertobin()\n");
    exit(1);
  }
  ret = BigNumber::toBin(bn_exponent_data_, length, exponent);
  if (!ret) {
    printf("bn_exponent_data_: failed at bignumbertobin()\n");
    exit(1);
  }
  ret = BigNumber::toBin(bn_modulus_data_, length, modulus);
  if (!ret) {
    printf("bn_modulus_data_: failed at bignumbertobin()\n");
    exit(1);
  }

  status = HE_QAT_bnModExp(bn_remainder_data_, bn_base_data_, bn_exponent_data_,
                           bn_modulus_data_, nbits);
  if (HE_QAT_STATUS_SUCCESS != status) {
    HE_QAT_PRINT_ERR("\nQAT bnModExp with BigNumber failed\n");
  }
  getBnModExpRequest(1);

  // Collect result and pack it into BigNumber
  BigNumber remainder;
  ret = BigNumber::fromBin(remainder, bn_remainder_data_, length);
  if (!ret) {
    printf("bn_remainder_data_: failed at bignumbertobin()\n");
    exit(1);
  }

  free(bn_base_data_);
  bn_base_data_ = NULL;
  free(bn_exponent_data_);
  bn_exponent_data_ = NULL;
  free(bn_modulus_data_);
  bn_modulus_data_ = NULL;
  free(bn_remainder_data_);
  bn_remainder_data_ = NULL;

  return remainder;
}
#endif  // IPCL_USE_QAT

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

std::vector<BigNumber> qatModExp(const std::vector<BigNumber>& base,
                                 const std::vector<BigNumber>& exp,
                                 const std::vector<BigNumber>& mod) {
#ifdef IPCL_USE_QAT
  // TODO(fdiasmor): Slice with custom batches, test OMP
  std::size_t worksize = base.size();
  std::vector<BigNumber> remainder(worksize);

  // remainder = heQatBnModExp(base, exp, mod);
  remainder = heQatBnModExp(base, exp, mod, 1024);
  return remainder;
#else
  ERROR_CHECK(false, "qatModExp: Need to turn on IPCL_ENABLE_QAT");
#endif  // IPCL_USE_QAT
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

std::vector<BigNumber> modExp(const std::vector<BigNumber>& base,
                              const std::vector<BigNumber>& exp,
                              const std::vector<BigNumber>& mod) {
#ifdef IPCL_USE_QAT
  return qatModExp(base, exp, mod);
#else
  return ippModExp(base, exp, mod);
#endif  // IPCL_USE_QAT
}

BigNumber modExp(const BigNumber& base, const BigNumber& exp,
                 const BigNumber& mod) {
  // QAT mod exp is NOT needed, when there is only 1 BigNumber.
  return ippModExp(base, exp, mod);
}

BigNumber ippModExp(const BigNumber& base, const BigNumber& exp,
                    const BigNumber& mod) {
  // IPP multi buffer mod exp is NOT needed, when there is only 1 BigNumber.
  return ippSBModExp(base, exp, mod);
}

}  // namespace ipcl
