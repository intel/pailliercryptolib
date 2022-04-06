// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/mod_exp.hpp"

#include <crypto_mb/exp.h>
#include <time.h>

#include <cstring>

#include "ipcl/util.hpp"

#ifdef IPCL_USE_QAT
#include <he_qat_bn_ops.h>
#include <he_qat_types.h>
#endif

namespace ipcl {

#ifdef IPCL_USE_QAT

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
  int length = dwords * sizeof(int64u);

  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    out_x[i] = reinterpret_cast<int64u*>(alloca(length));
    b_array[i] = reinterpret_cast<int64u*>(alloca(length));
    p_array[i] = reinterpret_cast<int64u*>(alloca(length));

    ERROR_CHECK(
        out_x[i] != nullptr && b_array[i] != nullptr && p_array[i] != nullptr,
        "ippMultiBuffExp: alloc memory for error");

    memset(out_x[i], 0, length);
    memset(b_array[i], 0, length);
    memset(p_array[i], 0, length);
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
  int nsqBitLen;
  int expBitLen = 0;

  for (int i = 0, bBitLen, pBitLen; i < IPCL_CRYPTO_MB_SIZE; i++) {
    ippsRef_BN(nullptr, &bBitLen, reinterpret_cast<Ipp32u**>(&pow_b[i]),
               base[i]);
    ippsRef_BN(nullptr, &pBitLen, reinterpret_cast<Ipp32u**>(&pow_p[i]),
               pow[i]);
    ippsRef_BN(nullptr, &nsqBitLen, &pow_nsquare[i], m[i]);

    memcpy(b_array[i], pow_b[i], BITSIZE_WORD(bBitLen) * 4);
    memcpy(p_array[i], pow_p[i], BITSIZE_WORD(pBitLen) * 4);

    if (expBitLen < pBitLen) expBitLen = pBitLen;
  }

  /*
   *Note: If actual sizes of exp are different, set the exp_bits parameter equal
   *to maximum size of the actual module in bit size and extend all the modules
   *with zero bits
   */
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
  BigNumber bn_c(m.front());

  std::vector<BigNumber> res(IPCL_CRYPTO_MB_SIZE, 0);
  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    bn_c.Set(reinterpret_cast<Ipp32u*>(out_x[i]), BITSIZE_WORD(nsqBitLen),
             IppsBigNumPOS);
    res[i] = bn_c;
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
  std::vector<BigNumber> remainder(IPCL_CRYPTO_MB_SIZE);
  remainder = heQatBnModExp(base, pow, m);
  return remainder;
#else
#ifdef IPCL_CRYPTO_MB_MOD_EXP
  return ippMBModExp(base, pow, m);
#else
  std::vector<BigNumber> res(IPCL_CRYPTO_MB_SIZE);
  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++)
    res[i] = ippSBModExp(base[i], pow[i], m[i]);
  return res;
#endif  // else
#endif  // IPCL_USE_QAT
}

BigNumber ippModExp(const BigNumber& base, const BigNumber& pow,
                    const BigNumber& m) {
#ifdef IPCL_USE_QAT
  return heQatBnModExp(base, pow, m);
#else
  return ippSBModExp(base, pow, m);
#endif  // IPCL_USE_QAT
}

}  // namespace ipcl
