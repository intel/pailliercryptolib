
#include "he_qat_misc.h"
#include "he_qat_utils.h"
#include "he_qat_bn_ops.h"
#include "he_qat_context.h"
#include "cpa_sample_utils.h"

//#ifdef __cplusplus
// extern "C" {
//#endif

#include <time.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>

//#ifdef __cplusplus
//}
//#endif

#include <string>

//#include "ippcp.h"
#include <iomanip>

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
        BIGNUM* bn_mod = generateTestBNData(bit_length);

        if (!bn_mod) continue;

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
