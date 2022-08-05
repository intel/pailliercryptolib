// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_COMMON_HPP_
#define IPCL_INCLUDE_IPCL_COMMON_HPP_

#include "ipcl/bignum.h"

namespace ipcl {

constexpr int IPCL_CRYPTO_MB_SIZE = 8;

/**
 * Get random value
 * @param[in] length bit length
 * @return the random value of type BigNumber
 */
BigNumber getRandomBN(int length);

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_COMMON_HPP_
