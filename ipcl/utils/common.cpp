// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/utils/common.hpp"

#include "crypto_mb/exp.h"
#include "ipcl/utils/util.hpp"

namespace ipcl {

IppStatus ippGenRandom(Ipp32u* rand, int bits, void* ctx) {
#ifdef IPCL_RUNTIME_DETECT_CPU_FEATURES
  if (has_rdseed)
    return ippsTRNGenRDSEED(rand, bits, ctx);
  else if (has_rdrand)
    return ippsPRNGenRDRAND(rand, bits, ctx);
  else
    return ippsPRNGen(rand, bits, ctx);
#else
#ifdef IPCL_RNG_INSTR_RDSEED
  return ippsTRNGenRDSEED(rand, bits, ctx);
#elif defined(IPCL_RNG_INSTR_RDRAND)
  return ippsPRNGenRDRAND(rand, bits, ctx);
#else
  return ippsPRNGen(rand, bits, ctx);
#endif
#endif  // IPCL_RUNTIME_IPP_RNG
}

IppStatus ippGenRandomBN(IppsBigNumState* rand, int bits, void* ctx) {
#ifdef IPCL_RUNTIME_DETECT_CPU_FEATURES
  if (has_rdseed)
    return ippsTRNGenRDSEED_BN(rand, bits, ctx);
  else if (has_rdrand)
    return ippsPRNGenRDRAND_BN(rand, bits, ctx);
  else
    return ippsPRNGen_BN(rand, bits, ctx);
#else
#ifdef IPCL_RNG_INSTR_RDSEED
  return ippsTRNGenRDSEED_BN(rand, bits, ctx);
#elif defined(IPCL_RNG_INSTR_RDRAND)
  return ippsPRNGenRDRAND_BN(rand, bits, ctx);
#else
  return ippsPRNGen_BN(rand, bits, ctx);
#endif
#endif  // IPCL_RUNTIME_IPP_RNG
}

BigNumber getRandomBN(int bits) {
  IppStatus stat;
  int bn_buf_size;

  int bn_len = BITSIZE_WORD(bits);
  stat = ippsBigNumGetSize(bn_len, &bn_buf_size);
  ERROR_CHECK(stat == ippStsNoErr,
              "getRandomBN: get IppsBigNumState context error.");

  IppsBigNumState* pBN =
      reinterpret_cast<IppsBigNumState*>(alloca(bn_buf_size));
  ERROR_CHECK(pBN != nullptr, "getRandomBN: big number alloca error");

  stat = ippsBigNumInit(bn_len, pBN);
  ERROR_CHECK(stat == ippStsNoErr,
              "getRandomBN: init big number context error.");

  stat = ippGenRandomBN(pBN, bits, NULL);
  ERROR_CHECK(stat == ippStsNoErr,
              "getRandomBN:  generate random big number error.");

  return BigNumber{pBN};
}

}  // namespace ipcl
