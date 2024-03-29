// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <time.h>
#include <omp.h>

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <cstring>
#include <iomanip>
#include <chrono>  // NOLINT [build/c++11]

#include "heqat/heqat.h"

const unsigned int BATCH_SIZE = 4096;

int main(int argc, const char** argv) {
    const int bit_length = 4096;
    const size_t num_trials = 20;

    double avg_speed_up = 0.0;
    double ssl_avg_time = 0.0;
    double qat_avg_time = 0.0;

    HE_QAT_STATUS status = HE_QAT_STATUS_FAIL;

    // Set up QAT runtime context
    acquire_qat_devices();

    // Set up OpenSSL context (as baseline)
    BN_CTX* ctx = BN_CTX_new();
    BN_CTX_start(ctx);

    int nthreads = 4;
    for (unsigned int mod = 0; mod < num_trials; mod++) {
        // Generate modulus number
        BIGNUM* bn_mod = generateTestBNData(bit_length);

        if (!bn_mod) continue;

        char* bn_str = BN_bn2hex(bn_mod);

        HE_QAT_PRINT_DBG("BIGNUM: %s num_bytes: %d num_bits: %d\n", bn_str,
                         BN_num_bytes(bn_mod), BN_num_bits(bn_mod));

        OPENSSL_free(bn_str);

        // Generate exponent in [0..bn_mod]
        BIGNUM* bn_exponent = BN_new();
        if (!BN_rand_range(bn_exponent, bn_mod)) {
            BN_free(bn_mod);
            continue;
        }

        // Generate base number
        BIGNUM* bn_base = generateTestBNData(bit_length);

        // Perform OpenSSL ModExp Op
        BIGNUM* ssl_res = BN_new();
        auto start = std::chrono::high_resolution_clock::now();
        BN_mod_exp(ssl_res, bn_base, bn_exponent, bn_mod, ctx);
        auto stop = std::chrono::high_resolution_clock::now();
        auto ssl_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        int len_ = (bit_length + 7) >> 3;

        // Start QAT timer (including data conversion overhead)
        start = std::chrono::high_resolution_clock::now();
        unsigned char* bn_base_data_ =
            (unsigned char*)calloc(len_, sizeof(unsigned char));
        if (NULL == bn_base_data_) exit(1);
        BN_bn2binpad(bn_base, bn_base_data_, len_);
        unsigned char* bn_mod_data_ =
            (unsigned char*)calloc(len_, sizeof(unsigned char));
        if (NULL == bn_mod_data_) exit(1);
        BN_bn2binpad(bn_mod, bn_mod_data_, len_);
        unsigned char* bn_exponent_data_ =
            (unsigned char*)calloc(len_, sizeof(unsigned char));
        if (NULL == bn_exponent_data_) exit(1);
        BN_bn2binpad(bn_exponent, bn_exponent_data_, len_);
        unsigned char* bn_remainder_data_ =
            (unsigned char*)calloc(nthreads * len_, sizeof(unsigned char));
        if (NULL == bn_remainder_data_) exit(1);
        stop = std::chrono::high_resolution_clock::now();
        auto cvt_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        // Simulate input number in BigNumber representation
        BigNumber big_num_base((Ipp32u)0);
        BigNumber big_num_mod((Ipp32u)0);
        BigNumber big_num_exponent((Ipp32u)0);
        status = binToBigNumber(big_num_base, bn_base_data_, bit_length);
        if (HE_QAT_STATUS_SUCCESS != status) {
            HE_QAT_PRINT_ERR("Failed at binToBigNumber()\n");
            exit(1);
        }
        status = binToBigNumber(big_num_mod, bn_mod_data_, bit_length);
        if (HE_QAT_STATUS_SUCCESS != status) {
            HE_QAT_PRINT_ERR("Failed at binToBigNumber()\n");
            exit(1);
        }
        status =
            binToBigNumber(big_num_exponent, bn_exponent_data_, bit_length);
        if (HE_QAT_STATUS_SUCCESS != status) {
            HE_QAT_PRINT_ERR("Failed at binToBigNumber()\n");
            exit(1);
        }

        // Reset numbers to 0
        memset(bn_base_data_, 0, len_);
        memset(bn_mod_data_, 0, len_);
        memset(bn_exponent_data_, 0, len_);
        // Make sure variables are reset
        if (memcmp(bn_base_data_, bn_mod_data_, len_) ||
            memcmp(bn_base_data_, bn_exponent_data_, len_)) {
            HE_QAT_PRINT_ERR("Pointers are not reset to zero!");
            exit(1);
        }

        // start = clock();
        start = std::chrono::high_resolution_clock::now();
        status = bigNumberToBin(bn_base_data_, bit_length, big_num_base);
        if (HE_QAT_STATUS_SUCCESS != status) {
            HE_QAT_PRINT_ERR("bn_base_data_: failed at bigNumberToBin()\n");
            exit(1);
        }
        status = bigNumberToBin(bn_mod_data_, bit_length, big_num_mod);
        if (HE_QAT_STATUS_SUCCESS != status) {
            HE_QAT_PRINT_ERR("bn_base_data_: failed at bigNumberToBin()\n");
            exit(1);
        }
        status =
            bigNumberToBin(bn_exponent_data_, bit_length, big_num_exponent);
        if (HE_QAT_STATUS_SUCCESS != status) {
            HE_QAT_PRINT_ERR("bn_base_data_: failed at bigNumberToBin()\n");
            exit(1);
        }
        // cvt_elapsed += (clock() - start);
        cvt_duration += std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start);

        omp_set_num_threads(nthreads);

        // Perform BigNumber modular exponentiation on QAT
        start = std::chrono::high_resolution_clock::now();

#pragma omp parallel private(status)
        {
            int thread_id = omp_get_thread_num();
            unsigned int buffer_id = thread_id;

            // Secure one of the distributed outstanding buffers
            status = acquire_bnModExp_buffer(&buffer_id);
            if (HE_QAT_STATUS_SUCCESS != status) {
                HE_QAT_PRINT_ERR("Failed to acquire_bnModExp_buffer()\n");
                exit(1);
            }

            HE_QAT_PRINT_DBG("Thread #%d HE QAT ACQUIRED BUFFER ID: %u\n",
                             thread_id, buffer_id);

            // Divide work among threads
            unsigned int worksize = BATCH_SIZE / nthreads;
            unsigned int begin = thread_id * worksize;
            unsigned int end = begin + worksize;

            HE_QAT_PRINT_DBG("Thread #%d Begin: %u End: %u\n", thread_id, begin,
                             end);

            // For local thread, schedule work execution
            for (unsigned int b = begin; b < end; b++)
                status = HE_QAT_bnModExp_MT(
                    buffer_id, bn_remainder_data_ + thread_id * len_,
                    bn_base_data_, bn_exponent_data_, bn_mod_data_, bit_length);

            HE_QAT_PRINT_DBG("Thread #%d Waiting\n", thread_id);

            // Wait for the request to complete
            release_bnModExp_buffer(buffer_id, BATCH_SIZE / nthreads);

            HE_QAT_PRINT_DBG("Thread #%d Completed\n", thread_id);
        }  // pragma omp parallel

        stop = std::chrono::high_resolution_clock::now();
        auto qat_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        ssl_avg_time =
            (mod * ssl_avg_time + (static_cast<double>(ssl_duration.count()))) /
            (mod + 1);
        qat_avg_time =
            (mod * qat_avg_time +
             (static_cast<double>(qat_duration.count())) / BATCH_SIZE) /
            (mod + 1);
        avg_speed_up =
            (mod * avg_speed_up +
             (ssl_duration.count() /
              static_cast<double>(qat_duration.count() / BATCH_SIZE))) /
            (mod + 1);

        HE_QAT_PRINT("Request #%u\t", mod + 1);
        HE_QAT_PRINT("Overhead: %.1luus", cvt_duration.count());
        HE_QAT_PRINT("\tOpenSSL: %.1lfus", ssl_avg_time);
        HE_QAT_PRINT("\tQAT: %.1lfus", qat_avg_time);
        HE_QAT_PRINT("\tSpeed-up: %.1lfx", avg_speed_up);

        BIGNUM* qat_res = BN_new();
        BN_bin2bn(bn_remainder_data_, len_, qat_res);

        if (HE_QAT_STATUS_SUCCESS != status) {
            HE_QAT_PRINT_ERR("\nQAT bnModExp with BigNumber failed\n");
            exit(1);
        }

        HE_QAT_PRINT_DBG("\nQAT bnModExpOp finished\n");

        // start = clock();
        BigNumber big_num((Ipp32u)0);
        status = binToBigNumber(big_num, bn_remainder_data_, bit_length);
        if (HE_QAT_STATUS_SUCCESS != status) {
            HE_QAT_PRINT_ERR(
                "bn_remainder_data_: Failed at bigNumberToBin()\n");
            exit(1);
        }

#ifdef HE_QAT_DEBUG
        bn_str = BN_bn2hex(qat_res);
        HE_QAT_PRINT_DBG("Bin: %s num_bytes(%d) num_bits(%d)\n", bn_str,
                         BN_num_bytes(qat_res), BN_num_bits(qat_res));
#endif

#ifdef HE_QAT_DEBUG
        int bit_len = 0;
        ippsRef_BN(NULL, &bit_len, NULL, BN(big_num));
        std::string str;
        big_num.num2hex(str);
        HE_QAT_PRINT_DBG("BigNumber:  %s num_bytes: %d num_bits: %d\n",
                         str.c_str(), len_, bit_len);
        HE_QAT_PRINT_DBG(
            "---------------------################-----------------------\n");
#endif

        if (BN_cmp(qat_res, ssl_res) != 0)
            HE_QAT_PRINT("\t** FAIL **\n");
        else
            HE_QAT_PRINT("\t** PASS **\n");

        BN_free(bn_mod);
        BN_free(bn_base);
        BN_free(bn_exponent);
        BN_free(qat_res);
        BN_free(ssl_res);

        free(bn_mod_data_);
        free(bn_base_data_);
        free(bn_exponent_data_);
        free(bn_remainder_data_);
    }

    // Tear down OpenSSL context
    BN_CTX_end(ctx);

    // Tear down QAT runtime context
    release_qat_devices();

    return static_cast<int>(status);
}
