// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <time.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <sys/time.h>

#include <string>

#include <iomanip>

#include "heqat/heqat.h"

struct timeval start_time, end_time;
double time_taken = 0.0;

int main(int argc, const char** argv) {
    const int bit_length = 1024;
    const size_t num_trials = 4;

    double ssl_elapsed = 0.0;
    double qat_elapsed = 0.0;

    HE_QAT_STATUS status = HE_QAT_STATUS_FAIL;

    // Set up OpenSSL context (as baseline)
    BN_CTX* ctx = BN_CTX_new();
    BN_CTX_start(ctx);

    for (unsigned int mod = 0; mod < num_trials; mod++) {
        BIGNUM* bn_mod = generateTestBNData(bit_length);

        if (!bn_mod) continue;

        char* bn_str = BN_bn2hex(bn_mod);
        HE_QAT_PRINT("BIGNUM: %s num_bytes: %d num_bits: %d\n", bn_str,
                     BN_num_bytes(bn_mod), BN_num_bits(bn_mod));
        OPENSSL_free(bn_str);

        int len_ = (bit_length + 7) >> 3;

        unsigned char* bn_mod_data_ =
            (unsigned char*)calloc(len_, sizeof(unsigned char));
        if (NULL == bn_mod_data_) exit(1);
        BN_bn2binpad(bn_mod, bn_mod_data_, len_);

        BN_free(bn_mod);

        BigNumber big_num((Ipp32u)0);

        gettimeofday(&start_time, NULL);
        status = binToBigNumber(big_num, bn_mod_data_, bit_length);
        if (HE_QAT_STATUS_SUCCESS != status) {
            HE_QAT_PRINT("Failed at binToBigNumber()\n");
            exit(1);
        }
        gettimeofday(&end_time, NULL);
        time_taken = (end_time.tv_sec - start_time.tv_sec) * 1e6;
        time_taken =
            (time_taken + (end_time.tv_usec - start_time.tv_usec));  //*1e-6;
        ssl_elapsed = time_taken;
        HE_QAT_PRINT("Conversion to BigNumber has completed in %.1lfus.\n",
                     (ssl_elapsed));

        int bit_len = 0;
        ippsRef_BN(NULL, &bit_len, NULL, BN(big_num));
        std::string str;
        big_num.num2hex(str);
        HE_QAT_PRINT("BigNumber:  %s num_bytes: %d num_bits: %d\n", str.c_str(),
                     len_, bit_len);

        gettimeofday(&start_time, NULL);
        unsigned char* ref_bn_data_ =
            (unsigned char*)calloc(len_, sizeof(unsigned char));
        if (NULL == ref_bn_data_) exit(1);
        status = bigNumberToBin(ref_bn_data_, bit_length, big_num);
        if (HE_QAT_STATUS_SUCCESS != status) {
            HE_QAT_PRINT("Failed at bigNumberToBin()\n");
            exit(1);
        }
        gettimeofday(&end_time, NULL);
        time_taken = (end_time.tv_sec - start_time.tv_sec) * 1e6;
        time_taken =
            (time_taken + (end_time.tv_usec - start_time.tv_usec));  //*1e-6;
        qat_elapsed = time_taken;
        HE_QAT_PRINT("Conversion from BigNumber has completed %.1lfus.\n",
                     (qat_elapsed));

        BIGNUM* ref_bin_ = BN_new();
        BN_bin2bn(ref_bn_data_, len_, ref_bin_);
        bn_str = BN_bn2hex(ref_bin_);
        HE_QAT_PRINT("Bin: %s num_bytes(%d) num_bits(%d)\n", bn_str,
                     BN_num_bytes(ref_bin_), BN_num_bits(ref_bin_));
        HE_QAT_PRINT("-----------------------\n");

        OPENSSL_free(bn_str);
        free(bn_mod_data_);
        free(ref_bn_data_);
        BN_free(ref_bin_);
    }

    // Tear down OpenSSL context
    BN_CTX_end(ctx);

    return static_cast<int>(status);
}
