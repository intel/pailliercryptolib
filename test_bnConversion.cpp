
#include "cpa_sample_utils.h"

#include "he_qat_misc.h"
#include "he_qat_bn_ops.h"
#include "he_qat_context.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#ifdef __cplusplus
}
#endif

#include <string>

#include "ippcp.h"
//#include "bignum.h"

//#include <iostream>
#include <iomanip>

//#define LEN_OF_1024_BITS 128
//#define LEN_OF_2048_BITS 256
//#define msb_CAN_BE_ZERO -1
//#define msb_IS_ONE 0
//#define EVEN_RND_NUM 0
//#define ODD_RND_NUM 1
//#define BATCH_SIZE 1

// using namespace std;

// int gDebugParam = 1;

// BIGNUM* generateTestBNData(int nbits) {
//    if (!RAND_status()) return NULL;
//#ifdef _DESTINY_DEBUG_VERBOSE
//    printf("PRNG properly seeded.\n");
//#endif
//
//    BIGNUM* bn = BN_new();
//
//    if (!BN_rand(bn, nbits, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY)) {
//        BN_free(bn);
//        printf("Error while generating BN random number: %lu\n",
//               ERR_get_error());
//        return NULL;
//    }
//
//    return bn;
//}
//
// unsigned char* paddingZeros(BIGNUM* bn, int nbits) {
//    if (!bn) return NULL;
//
//    // Returns same address if it fails
//    int num_bytes = BN_num_bytes(bn);
//    int bytes_left = nbits / 8 - num_bytes;
//    if (bytes_left <= 0) return NULL;
//
//    // Returns same address if it fails
//    unsigned char* bin = NULL;
//    int len = bytes_left + num_bytes;
//    if (!(bin = (unsigned char*)OPENSSL_zalloc(len))) return NULL;
//
//#ifdef _DESTINY_DEBUG_VERBOSE
//    printf("Padding bn with %d bytes to total %d bytes\n", bytes_left, len);
//#endif
//    BN_bn2binpad(bn, bin, len);
//    if (ERR_get_error()) {
//        OPENSSL_free(bin);
//        return NULL;
//    }
//
//    return bin;
//}
//
// void showHexBN(BIGNUM* bn, int nbits) {
//    int len = nbits / 8;
//    unsigned char* bin = (unsigned char*)OPENSSL_zalloc(len);
//    if (!bin) return;
//    if (BN_bn2binpad(bn, bin, len)) {
//        for (size_t i = 0; i < len; i++) printf("%d", bin[i]);
//        printf("\n");
//    }
//    OPENSSL_free(bin);
//    return;
//}
//
// void showHexBin(unsigned char* bin, int len) {
//    if (!bin) return;
//    for (size_t i = 0; i < len; i++) printf("%d", bin[i]);
//    printf("\n");
//    return;
//}
//
///// @brief
///// @function binToBigNumber
////
///// data will be changed to little endian format in this function, therefore
///the
///// abscence of const in front
// HE_QAT_STATUS binToBigNumber(BigNumber& bn, const unsigned char* data,
//                             int nbits) {
//    if (nbits <= 0) return HE_QAT_STATUS_INVALID_PARAM;
//    int len_ = (nbits + 7) >> 3;  // nbits/8;
//
//    // Create BigNumber containg input data passed as argument
//    bn = BigNumber(reinterpret_cast<const Ipp32u*>(data),
//    BITSIZE_WORD(nbits)); Ipp32u* ref_bn_data_ = NULL; ippsRef_BN(NULL, NULL,
//    &ref_bn_data_, BN(bn));
//
//    // Convert it to little endian format
//    unsigned char* data_ = reinterpret_cast<unsigned char*>(ref_bn_data_);
//    for (int i = 0; i < len_; i++) data_[i] = data[len_ - 1 - i];
//
//    return HE_QAT_STATUS_SUCCESS;
//}
//
///// @brief
///// @function bigNumberToBin
///// Converts/encapsulates big number data to/into an object of BigNumber's
///// type/class.
///// @param[out] data  BigNumber object's raw data in big endian format.
///// @param[in]  nbits Number of bits. Has to be power of 2, e.g. 1024, 2048,
///// 4096, etc.
///// @param[in]  bn    BigNumber object holding a multi-precision that can be
///// represented in nbits.
// HE_QAT_STATUS bigNumberToBin(unsigned char* data, int nbits,
//                             const BigNumber& bn) {
//    if (nbits <= 0) return HE_QAT_STATUS_INVALID_PARAM;
//    int len_ = (nbits + 7) >> 3;  // nbits/8;
//
//    // Extract raw vector of data in little endian format
//    Ipp32u* ref_bn_data_ = NULL;
//    ippsRef_BN(NULL, NULL, &ref_bn_data_, BN(bn));
//
//    // Revert it to big endian format
//    unsigned char* data_ = reinterpret_cast<unsigned char*>(ref_bn_data_);
//    for (int i = 0; i < len_; i++) data[i] = data_[len_ - 1 - i];
//
//    return HE_QAT_STATUS_SUCCESS;
//}

int main(int argc, const char** argv) {
    const int bit_length = 1024;
    const size_t num_trials = 4;

    double avg_speed_up = 0.0;
    double ssl_avg_time = 0.0;
    double qat_avg_time = 0.0;

    clock_t start = CLOCKS_PER_SEC;
    clock_t ssl_elapsed = CLOCKS_PER_SEC;
    clock_t qat_elapsed = CLOCKS_PER_SEC;

    HE_QAT_STATUS status = HE_QAT_STATUS_FAIL;

    // Set up QAT runtime context
    acquire_qat_devices();

    // Set up OpenSSL context (as baseline)
    BN_CTX* ctx = BN_CTX_new();
    BN_CTX_start(ctx);

    for (unsigned int mod = 0; mod < num_trials; mod++) {
        // BIGNUM* bn_mod = BN_new();
        BIGNUM* bn_mod = generateTestBNData(bit_length);

        if (!bn_mod) continue;

        // BN_set_word(bn_mod, mod);

        char* bn_str = BN_bn2hex(bn_mod);
        printf("BIGNUM: %s num_bytes: %d num_bits: %d\n", bn_str,
               BN_num_bytes(bn_mod), BN_num_bits(bn_mod));
        OPENSSL_free(bn_str);

        int len_ = (bit_length + 7) >> 3;

        unsigned char* bn_mod_data_ =
            (unsigned char*)calloc(len_, sizeof(unsigned char));
        if (NULL == bn_mod_data_) exit(1);
        BN_bn2binpad(bn_mod, bn_mod_data_, len_);

        BN_free(bn_mod);

        BigNumber big_num((Ipp32u)0);

        start = clock();
        status = binToBigNumber(big_num, bn_mod_data_, bit_length);
        if (HE_QAT_STATUS_SUCCESS != status) {
            printf("Failed at binToBigNumber()\n");
            exit(1);
        }
        ssl_elapsed = clock() - start;
        printf("Conversion to BigNumber has completed in %.1lfus.\n",
               (ssl_elapsed / (CLOCKS_PER_SEC / 1000000.0)));

        int bit_len = 0;
        ippsRef_BN(NULL, &bit_len, NULL, BN(big_num));
        std::string str;
        big_num.num2hex(str);
        printf("BigNumber:  %s num_bytes: %d num_bits: %d\n", str.c_str(), len_,
               bit_len);

        start = clock();
        unsigned char* ref_bn_data_ =
            (unsigned char*)calloc(len_, sizeof(unsigned char));
        if (NULL == ref_bn_data_) exit(1);
        status = bigNumberToBin(ref_bn_data_, bit_length, big_num);
        if (HE_QAT_STATUS_SUCCESS != status) {
            printf("Failed at bigNumberToBin()\n");
            exit(1);
        }
        qat_elapsed = clock() - start;
        printf("Conversion from BigNumber has completed %.1lfus.\n",
               (qat_elapsed / (CLOCKS_PER_SEC / 1000000.0)));

        BIGNUM* ref_bin_ = BN_new();
        BN_bin2bn(ref_bn_data_, len_, ref_bin_);
        bn_str = BN_bn2hex(ref_bin_);
        printf("Bin: %s num_bytes(%d) num_bits(%d)\n", bn_str,
               BN_num_bytes(ref_bin_), BN_num_bits(ref_bin_));
        printf("-----------------------\n");

        OPENSSL_free(bn_str);
        free(bn_mod_data_);
        free(ref_bn_data_);
        BN_free(ref_bin_);
    }

    // Tear down OpenSSL context
    BN_CTX_end(ctx);

    // Tear down QAT runtime context
    release_qat_devices();

    return (int)status;
}
