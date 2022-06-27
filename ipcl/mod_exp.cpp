// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/mod_exp.hpp"

#include <crypto_mb/exp.h>

#include <algorithm>
#include <cstring>
#include <iostream>

#ifdef IPCL_USE_QAT
#include <he_qat_bn_ops.h>
#include <he_qat_types.h>
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
        PRINT_ERR("\nQAT bnModExp with BigNumber failed\n");
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
        PRINT_ERR("\nQAT bnModExp with BigNumber failed\n");
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
      PRINT_ERR("\nQAT bnModExp with BigNumber failed\n");
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
    PRINT_ERR("\nQAT bnModExp with BigNumber failed\n");
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
#ifdef IPCL_USE_QAT
  // TODO(fdiasmor): Slice with custom batches, test OMP
  std::size_t worksize = base.size();
  std::vector<BigNumber> remainder(worksize);
  // remainder = heQatBnModExp(base, pow, m);
  remainder = heQatBnModExp(base, pow, m, 1024);
  return remainder;
#else
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
  int omp_remaining_threads = OMPUtilities::MaxThreads;
#pragma omp parallel for num_threads( \
    OMPUtilities::assignOMPThreads(omp_remaining_threads, num_chunk))
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
  int omp_remaining_threads = OMPUtilities::MaxThreads;
#pragma omp parallel for num_threads( \
    OMPUtilities::assignOMPThreads(omp_remaining_threads, v_size))
#endif  // IPCL_USE_OMP
  for (int i = 0; i < v_size; i++) res[i] = ippSBModExp(base[i], pow[i], m[i]);
  return res;

#endif  // IPCL_CRYPTO_MB_MOD_EXP
#endif  // IPCL_USE_QAT
}

BigNumber ippModExp(const BigNumber& base, const BigNumber& pow,
                    const BigNumber& m) {
  return ippSBModExp(base, pow, m);
}

}  // namespace ipcl
