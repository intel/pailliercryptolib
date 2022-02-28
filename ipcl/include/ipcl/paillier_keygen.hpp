// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_PAILLIER_KEYGEN_HPP_
#define IPCL_INCLUDE_IPCL_PAILLIER_KEYGEN_HPP_

#include "ipcl/paillier_prikey.hpp"
#include "ipcl/paillier_pubkey.hpp"

/**
 * Paillier key structure contains a public key and private key
 * pub_key: paillier public key
 * priv_key: paillier private key
 */
struct keyPair {
  PaillierPublicKey* pub_key;
  PaillierPrivateKey* priv_key;
};

/**
 * Generate Prime Big Number
 * @param[in] maxBitSize Maxmuim bit length of generated Prime
 */
BigNumber getPrimeBN(int maxBitSize);

/**
 * Generate a pair of key
 * @param[in] n_length Bit length of key size
 * @param[in] enable_DJN Default enabled DJN
 */
keyPair generateKeypair(int64_t n_length, bool enable_DJN = true);

#endif  // IPCL_INCLUDE_IPCL_PAILLIER_KEYGEN_HPP_
