/// @file heqat/misc/misc.cpp
// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "heqat/misc.h"

#include <openssl/err.h>
#include <openssl/rand.h>

HE_QAT_STATUS binToBigNumber(BigNumber& bn, const unsigned char* data,
                             int nbits) {
    if (nbits <= 0) return HE_QAT_STATUS_INVALID_PARAM;
    int len_ = (nbits + 7) >> 3;  // nbits/8;

    // Create BigNumber containg input data passed as argument
    bn = BigNumber(reinterpret_cast<const Ipp32u*>(data), BITSIZE_WORD(nbits));
    Ipp32u* ref_bn_data_ = NULL;
    ippsRef_BN(NULL, NULL, &ref_bn_data_, BN(bn));

    // Convert it to little endian format
    unsigned char* data_ = reinterpret_cast<unsigned char*>(ref_bn_data_);
    for (int i = 0; i < len_; i++) data_[i] = data[len_ - 1 - i];

    return HE_QAT_STATUS_SUCCESS;
}

HE_QAT_STATUS bigNumberToBin(unsigned char* data, int nbits,
                             const BigNumber& bn) {
    if (nbits <= 0) return HE_QAT_STATUS_INVALID_PARAM;
    int len_ = (nbits + 7) >> 3;  // nbits/8;

    // Extract raw vector of data in little endian format
    Ipp32u* ref_bn_data_ = NULL;
    ippsRef_BN(NULL, NULL, &ref_bn_data_, BN(bn));

    // Revert it to big endian format
    unsigned char* data_ = reinterpret_cast<unsigned char*>(ref_bn_data_);
    for (int i = 0; i < len_; i++) data[i] = data_[len_ - 1 - i];

    return HE_QAT_STATUS_SUCCESS;
}

