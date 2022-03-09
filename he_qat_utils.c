#include "he_qat_utils.h"

#ifdef __cplusplus
#include "utils/bignum.h"
#endif

#include <openssl/err.h>
#include <openssl/rand.h>

BIGNUM* generateTestBNData(int nbits) {
    if (!RAND_status()) return NULL;
#ifdef _DESTINY_DEBUG_VERBOSE
    printf("PRNG properly seeded.\n");
#endif

    BIGNUM* bn = BN_new();

    if (!BN_rand(bn, nbits, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY)) {
        BN_free(bn);
        printf("Error while generating BN random number: %lu\n",
               ERR_get_error());
        return NULL;
    }

    return bn;
}

unsigned char* paddingZeros(BIGNUM* bn, int nbits) {
    if (!bn) return NULL;

    // Returns same address if it fails
    int num_bytes = BN_num_bytes(bn);
    int bytes_left = nbits / 8 - num_bytes;
    if (bytes_left <= 0) return NULL;

    // Returns same address if it fails
    unsigned char* bin = NULL;
    int len = bytes_left + num_bytes;
    if (!(bin = OPENSSL_zalloc(len))) return NULL;

#ifdef _DESTINY_DEBUG_VERBOSE
    printf("Padding bn with %d bytes to total %d bytes\n", bytes_left, len);
#endif
    BN_bn2binpad(bn, bin, len);
    if (ERR_get_error()) {
        OPENSSL_free(bin);
        return NULL;
    }

    return bin;
}

void showHexBN(BIGNUM* bn, int nbits) {
    int len = nbits / 8;
    unsigned char* bin = OPENSSL_zalloc(len);
    if (!bin) return;
    if (BN_bn2binpad(bn, bin, len)) {
        for (size_t i = 0; i < len; i++) printf("%d", bin[i]);
        printf("\n");
    }
    OPENSSL_free(bin);
    return;
}

void showHexBin(unsigned char* bin, int len) {
    if (!bin) return;
    for (size_t i = 0; i < len; i++) printf("%d", bin[i]);
    printf("\n");
    return;
}

#ifdef __cplusplus
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

}  // extern "C" {
#endif

#endif  // HE_QAT_UTILS_H_
