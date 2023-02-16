// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/utils/common.hpp"

#include "crypto_mb/exp.h"
#include "ipcl/utils/util.hpp"

namespace ipcl {

void rand32u(std::vector<Ipp32u>& addr) {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);
  for (auto& x : addr) x = (dist(rng) << 16) + dist(rng);
}

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
    int seed_size = 160;
    int size;
    ippsPRNGGetSize(&size);
    auto prng = std::vector<Ipp8u>(size);
    ippsPRNGInit(seed_size, reinterpret_cast<IppsPRNGState*>(prng.data()));

    auto seed = std::vector<Ipp32u>(seed_size);
    rand32u(seed);
    BigNumber seed_bn(seed.data(), seed_size, IppsBigNumPOS);
    ippsPRNGSetSeed(BN(seed_bn), reinterpret_cast<IppsPRNGState*>(prng.data()));
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
