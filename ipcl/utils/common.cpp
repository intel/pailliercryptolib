// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/utils/common.hpp"

#include "crypto_mb/exp.h"
#include "ipcl/utils/util.hpp"

namespace ipcl {

IppStatus ippGenRandom(Ipp32u* rand, int bits, void* ctx) {
  if (kRNGenType == RNGenType::RDSEED)
    return ippsTRNGenRDSEED(rand, bits, ctx);
  else if (kRNGenType == RNGenType::RDRAND)
    return ippsPRNGenRDRAND(rand, bits, ctx);
  else if (kRNGenType == RNGenType::PSEUDO)
    return ippsPRNGen(rand, bits, ctx);
  else
    ERROR_CHECK(false, "ippGenRandom: RNGenType does not exist.");
}

IppStatus ippGenRandomBN(IppsBigNumState* rand, int bits, void* ctx) {
  if (kRNGenType == RNGenType::RDSEED) {
    return ippsTRNGenRDSEED_BN(rand, bits, ctx);
  } else if (kRNGenType == RNGenType::RDRAND) {
    return ippsPRNGenRDRAND_BN(rand, bits, ctx);
  } else if (kRNGenType == RNGenType::PSEUDO) {
    int size;
    ippsPRNGGetSize(&size);
    auto prng = std::vector<Ipp8u>(size);
    ippsPRNGInit(160, reinterpret_cast<IppsPRNGState*>(prng.data()));
    return ippsPRNGen_BN(rand, bits,
                         reinterpret_cast<IppsPRNGState*>(prng.data()));
  } else {
    ERROR_CHECK(false, "ippGenRandomBN: RNGenType does not exist.");
  }
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
