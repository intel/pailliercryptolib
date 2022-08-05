
#include "he_qat_bn_ops.h"
#include "he_qat_context.h"
#include "cpa_sample_utils.h"

#include <time.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#define LEN_OF_1024_BITS 128
#define LEN_OF_2048_BITS 256
#define msb_CAN_BE_ZERO -1
#define msb_IS_ONE 0
#define EVEN_RND_NUM 0
#define ODD_RND_NUM 1
#define BATCH_SIZE 1

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

int main(int argc, const char** argv) {
    const int bit_length = 4096;  // 1024;
    const size_t num_trials = 100;

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

    for (size_t mod = 0; mod < num_trials; mod++) {
        BIGNUM* bn_mod = generateTestBNData(bit_length);

        if (!bn_mod) continue;

#ifdef _DESTINY_DEBUG_VERBOSE
        char* bn_str = BN_bn2hex(bn_mod);
        printf("Generated modulus: %s num_bytes: %d num_bits: %d\n", bn_str,
               BN_num_bytes(bn_mod), BN_num_bits(bn_mod));
        OPENSSL_free(bn_str);
#endif
        // bn_exponent in [0..bn_mod]
        BIGNUM* bn_exponent = BN_new();
        if (!BN_rand_range(bn_exponent, bn_mod)) {
            BN_free(bn_mod);
            continue;
        }

        BIGNUM* bn_base = generateTestBNData(bit_length);

        // Perform OpenSSL ModExp Op
        BIGNUM* ssl_res = BN_new();
        start = clock();
        BN_mod_exp(ssl_res, bn_base, bn_exponent, bn_mod, ctx);
        ssl_elapsed = clock() - start;

        if (!ERR_get_error()) {
#ifdef _DESTINY_DEBUG_VERBOSE
            bn_str = BN_bn2hex(ssl_res);
            printf("SSL BN mod exp: %s num_bytes: %d num_bits: %d\n", bn_str,
                   BN_num_bytes(ssl_res), BN_num_bits(ssl_res));
            showHexBN(ssl_res, bit_length);
            OPENSSL_free(bn_str);
#endif
        } else {
            printf("Modular exponentiation failed.\n");
        }

#ifdef _DESTINY_DEBUG_VERBOSE
        PRINT_DBG("\nStarting QAT bnModExp...\n");
#endif

        //        printf("OpenSSL: %.1lfus\t", ssl_elapsed / (CLOCKS_PER_SEC /
        //        1000000.0));

        // Perform QAT ModExp Op
        BIGNUM* qat_res = BN_new();
        start = clock();
        for (unsigned int j = 0; j < BATCH_SIZE; j++)
            status = bnModExpPerformOp(qat_res, bn_base, bn_exponent, bn_mod,
                                       bit_length);
        getBnModExpRequest(BATCH_SIZE);
        qat_elapsed = clock() - start;

        ssl_avg_time = (mod * ssl_avg_time +
                        (ssl_elapsed / (CLOCKS_PER_SEC / 1000000.0))) /
                       (mod + 1);
        qat_avg_time =
            (mod * qat_avg_time +
             (qat_elapsed / (CLOCKS_PER_SEC / 1000000.0)) / BATCH_SIZE) /
            (mod + 1);
        avg_speed_up =
            (mod * avg_speed_up +
             (ssl_elapsed / (CLOCKS_PER_SEC / 1000000.0)) /
                 ((qat_elapsed / (CLOCKS_PER_SEC / 1000000.0)) / BATCH_SIZE)) /
            (mod + 1);

        printf(
            "Trial #%03lu\tOpenSSL: %.1lfus\tQAT: %.1lfus\tSpeed Up:%.1lfx\t",
            (mod + 1), ssl_avg_time, qat_avg_time, avg_speed_up);

        //        printf("QAT: %.1lfus\t", qat_elapsed / (CLOCKS_PER_SEC /
        //        1000000.0)); printf("Speed Up: %.1lfx\t", (ssl_elapsed /
        //        (CLOCKS_PER_SEC / 1000000.0) / BATCH_SIZE) / (qat_elapsed /
        //        (CLOCKS_PER_SEC / 1000000.0) / BATCH_SIZE) );

        if (HE_QAT_STATUS_SUCCESS != status) {
            PRINT_ERR("\nQAT bnModExpOp failed\n");
        }
#ifdef _DESTINY_DEBUG_VERBOSE
        else {
            PRINT_DBG("\nQAT bnModExpOp finished\n");
        }
#endif

        if (BN_cmp(qat_res, ssl_res) != 0)
            printf("\t** FAIL **\n");
        else
            printf("\t** PASS **\n");

        BN_free(ssl_res);
        BN_free(qat_res);

        BN_free(bn_mod);
        BN_free(bn_base);
        BN_free(bn_exponent);
    }

    // Tear down OpenSSL context
    BN_CTX_end(ctx);

    // Tear down QAT runtime context
    release_qat_devices();

    return (int)status;
}
