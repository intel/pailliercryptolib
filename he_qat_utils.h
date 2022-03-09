
#ifndef HE_QAT_UTILS_H_
#define HE_QAT_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/bn.h>

#define LEN_OF_1024_BITS 128
#define LEN_OF_2048_BITS 256
#define msb_CAN_BE_ZERO -1
#define msb_IS_ONE 0
#define EVEN_RND_NUM 0
#define ODD_RND_NUM 1
#define BATCH_SIZE 1

BIGNUM* generateTestBNData(int nbits);
unsigned char* paddingZeros(BIGNUM* bn, int nbits);
void showHexBN(BIGNUM* bn, int nbits); 
void showHexBin(unsigned char* bin, int len);

#ifdef __cplusplus
/// @brief
/// @function binToBigNumber
/// Converts/encapsulates QAT's raw large number data to/into a BigNumber object.
/// data will be changed to little endian format in this function, therefore the
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
} // extern "C" {
#endif

#endif // HE_QAT_UTILS_H_
