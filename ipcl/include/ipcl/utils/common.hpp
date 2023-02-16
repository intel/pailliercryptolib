// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_UTILS_COMMON_HPP_
#define IPCL_INCLUDE_IPCL_UTILS_COMMON_HPP_

#include <climits>
#include <random>
#include <vector>

#include "ipcl/bignum.h"

namespace ipcl {

constexpr int IPCL_CRYPTO_MB_SIZE = 8;
constexpr int IPCL_QAT_MODEXP_BATCH_SIZE = 1024;

constexpr int IPCL_WORKLOAD_SIZE_THRESHOLD = 128;

constexpr float IPCL_HYBRID_MODEXP_RATIO_FULL = 1.0;
constexpr float IPCL_HYBRID_MODEXP_RATIO_ENCRYPT = 0.25;
constexpr float IPCL_HYBRID_MODEXP_RATIO_DECRYPT = 0.12;
constexpr float IPCL_HYBRID_MODEXP_RATIO_MULTIPLY = 0.18;

/**
 * Generate random number with std mt19937
 * @param[in,out] addr Location used to store the generated random number
 */
void rand32u(std::vector<Ipp32u>& addr);

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
#endif  // IPCL_INCLUDE_IPCL_UTILS_COMMON_HPP_
