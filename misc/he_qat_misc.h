
#ifndef HE_QAT_MISC_H_
#define HE_QAT_MISC_H_

#include "he_qat_types.h"
#include "bignum.h"
#include <openssl/bn.h>

/// @brief
/// @function binToBigNumber
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
/// @function bigNumberToBin
/// Convert BigNumber object into raw data compatible with QAT.
/// @param[out] data  BigNumber object's raw data in big endian format.
/// @param[in]  nbits Number of bits. Has to be power of 2, e.g. 1024, 2048,
/// 4096, etc.
/// @param[in]  bn    BigNumber object holding a multi-precision that can be
/// represented in nbits.
HE_QAT_STATUS bigNumberToBin(unsigned char* data, int nbits,
                             const BigNumber& bn);

#endif  // HE_QAT_MISC_H_
