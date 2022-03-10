// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/mod_exp.hpp"
#include "ipcl/util.hpp"

#include <crypto_mb/exp.h>

#include <cstring>

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

  // encode base into Montfomery form
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
#ifdef IPCL_CRYPTO_MB_MOD_EXP
  return ippMBModExp(base, pow, m);
#else
  std::vector<BigNumber> res(IPCL_CRYPTO_MB_SIZE);
#ifdef IPCL_USE_OMP
#pragma omp parallel for
#endif
  for (int i = 0; i < IPCL_CRYPTO_MB_SIZE; i++) {
    res[i] = ippSBModExp(base[i], pow[i], m[i]);
  }
  return res;
#endif
}

BigNumber ippModExp(const BigNumber& base, const BigNumber& pow,
                    const BigNumber& m) {
  return ippSBModExp(base, pow, m);
}

}  // namespace ipcl
