// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_COMMON_HPP_
#define IPCL_INCLUDE_IPCL_COMMON_HPP_

#include "ipcl/bignum.h"

namespace ipcl {

constexpr int IPCL_CRYPTO_MB_SIZE = 8;

/**
 * Random generator wrapper.Generates a random unsigned Big Number of the
 * specified bit length
 * @param[in] rand Pointer to the output unsigned integer big number
 * @param[in] bits The number of generated bits
 * @param[in] ctx Pointer to the IppsPRNGState context.
 * @return Error code
 */
IppStatus ippGenRandom(Ipp32u* rand, int bits, void* ctx);

/**
 * Random generator wrapper.Generates a random positive Big Number of the
 * specified bit length
 * @param[in] rand Pointer to the output Big Number
 * @param[in] bits The number of generated bits
 * @param[in] ctx Pointer to the IppsPRNGState context.
 * @return Error code
 */
IppStatus ippGenRandomBN(IppsBigNumState* rand, int bits, void* ctx);

/**
 * Get random value
 * @param[in] bits The number of Big Number bits
 * @return The random value of type Big Number
 */
BigNumber getRandomBN(int bits);

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_COMMON_HPP_
