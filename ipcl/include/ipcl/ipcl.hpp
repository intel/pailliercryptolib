// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_IPCL_HPP_
#define IPCL_INCLUDE_IPCL_IPCL_HPP_

#include "ipcl/mod_exp.hpp"
#include "ipcl/pri_key.hpp"
#include "ipcl/utils/context.hpp"
#include "ipcl/utils/serialize.hpp"

namespace ipcl {

/**
 * Paillier key structure contains a public key and private key
 * pk: paillier public key
 * sk: paillier private key
 */
struct KeyPair {
  PublicKey pk;
  PrivateKey sk;
};

/**
 * Generate prime number
 * @param[in] maxBitSize Maximum bit length of to be generated prime number
 * @return The function return a prime big number
 */
BigNumber getPrimeBN(int maxBitSize);

/**
 * Generate a public/private key pair
 * @param[in] n_length Bit length of key size
 * @param[in] enable_DJN Enable DJN (default=true)
 * @return The function return the public and private key pair
 */
KeyPair generateKeypair(int64_t n_length, bool enable_DJN = true);

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_IPCL_HPP_
