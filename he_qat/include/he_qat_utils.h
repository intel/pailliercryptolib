
#ifndef HE_QAT_UTILS_H_
#define HE_QAT_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/bn.h>

BIGNUM* generateTestBNData(int nbits);
unsigned char* paddingZeros(BIGNUM* bn, int nbits);
void showHexBN(BIGNUM* bn, int nbits);
void showHexBin(unsigned char* bin, int len);

#ifdef __cplusplus
}  // extern "C" {
#endif

#endif  // HE_QAT_UTILS_H_
