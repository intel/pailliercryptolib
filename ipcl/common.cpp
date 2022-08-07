// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/common.hpp"

#include <crypto_mb/exp.h>

#include "ipcl/util.hpp"

namespace ipcl {

BigNumber getRandomBN(int bit_len) {
  IppStatus stat;
  int bn_buf_size;

  int bn_len = BITSIZE_WORD(bit_len);
  stat = ippsBigNumGetSize(bn_len, &bn_buf_size);
  ERROR_CHECK(stat == ippStsNoErr,
              "getRandomBN: get IppsBigNumState context error.");

  IppsBigNumState* pBN =
      reinterpret_cast<IppsBigNumState*>(alloca(bn_buf_size));
  ERROR_CHECK(pBN != nullptr, "getRandomBN: big number alloca error");

  stat = ippsBigNumInit(bn_len, pBN);
  ERROR_CHECK(stat == ippStsNoErr,
              "getRandomBN: init big number context error.");

#ifdef IPCL_RNG_INSTR_RDSEED
  ippsTRNGenRDSEED_BN(pBN, bit_len, NULL);
#elif defined(IPCL_RNG_INSTR_RDRAND)
  ippsPRNGenRDRAND_BN(pBN, bit_len, NULL);
#else
  ippsPRNGen_BN(pBN, bit_len, NULL);
#endif

  return BigNumber{pBN};
}

}  // namespace ipcl
