// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
/// @file heqat/misc.h

#pragma once

#ifndef HE_QAT_MISC_H_
#define HE_QAT_MISC_H_

#include "heqat/common/consts.h" 
#include "heqat/common/types.h"

#ifdef __cplusplus

#include "heqat/misc/bignum.h"

/// @brief
/// Convert QAT large number into little endian format and encapsulate it into a
/// BigNumber object.
/// @param[out] bn   BigNumber object representing multi-precision number in
/// little endian format.
/// @param[in]  data Large number of nbits precision in big endian format.
/// @param[in]  nbits Number of bits. Has to be power of 2, e.g. 1024, 2048,
/// 4096, etc.
HE_QAT_STATUS binToBigNumber(BigNumber& bn, const unsigned char* data,
                             int nbits);
/// @brief
/// Convert BigNumber object into raw data compatible with QAT.
/// @param[out] data  BigNumber object's raw data in big endian format.
/// @param[in]  nbits Number of bits. Has to be power of 2, e.g. 1024, 2048,
/// 4096, etc.
/// @param[in]  bn    BigNumber object holding a multi-precision that can be
/// represented in nbits.
HE_QAT_STATUS bigNumberToBin(unsigned char* data, int nbits,
                             const BigNumber& bn);
#endif // __cpluscplus

#endif  // HE_QAT_MISC_H_
