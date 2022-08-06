
#include "he_qat_misc.h"
#include "he_qat_utils.h"
#include "he_qat_bn_ops.h"
#include "he_qat_context.h"
#include "cpa_sample_utils.h"

#include <chrono>
#include <time.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <string>
#include <iomanip>
#include <omp.h>

#ifdef _DESTINY_DEBUG_VERBOSE
int gDebugParam = 1;
#endif

const unsigned int BATCH_SIZE = 2048;

using namespace std::chrono;

int main(int argc, const char** argv) {
    const int bit_length = 4096;
    const size_t num_trials = 10000;

    double avg_speed_up = 0.0;
    double ssl_avg_time = 0.0;
    double qat_avg_time = 0.0;

    //    clock_t start = CLOCKS_PER_SEC;
    //    clock_t ssl_elapsed = CLOCKS_PER_SEC;
    //    clock_t qat_elapsed = CLOCKS_PER_SEC;

    HE_QAT_STATUS status = HE_QAT_STATUS_FAIL;

    // Set up QAT runtime context
    acquire_qat_devices();

    // Set up OpenSSL context (as baseline)
    BN_CTX* ctx = BN_CTX_new();
    BN_CTX_start(ctx);

    int nthreads = 2;
    for (unsigned int mod = 0; mod < num_trials; mod++) {
        // Generate modulus number
        BIGNUM* bn_mod = generateTestBNData(bit_length);

        if (!bn_mod) continue;

        char* bn_str = BN_bn2hex(bn_mod);
#ifdef _DESTINY_DEBUG_VERBOSE
        printf("BIGNUM: %s num_bytes: %d num_bits: %d\n", bn_str,
               BN_num_bytes(bn_mod), BN_num_bits(bn_mod));
#endif
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
        auto start = high_resolution_clock::now();
        // start = clock();
        BN_mod_exp(ssl_res, bn_base, bn_exponent, bn_mod, ctx);
        auto stop = high_resolution_clock::now();
        auto ssl_duration = duration_cast<microseconds>(stop - start);
        // ssl_elapsed = clock() - start;

        int len_ = (bit_length + 7) >> 3;

        // Start QAT timer (including data conversion overhead)
        //        start = clock();
        start = high_resolution_clock::now();
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
        stop = high_resolution_clock::now();
        auto cvt_duration = duration_cast<microseconds>(stop - start);
        //	clock_t cvt_elapsed = clock() - start;

        // Simulate input number in BigNumber representation
        BigNumber big_num_base((Ipp32u)0);
        BigNumber big_num_mod((Ipp32u)0);
        BigNumber big_num_exponent((Ipp32u)0);
        status = binToBigNumber(big_num_base, bn_base_data_, bit_length);
        if (HE_QAT_STATUS_SUCCESS != status) {
            printf("Failed at binToBigNumber()\n");
            exit(1);
        }
        status = binToBigNumber(big_num_mod, bn_mod_data_, bit_length);
        if (HE_QAT_STATUS_SUCCESS != status) {
            printf("Failed at binToBigNumber()\n");
            exit(1);
        }
        status =
            binToBigNumber(big_num_exponent, bn_exponent_data_, bit_length);
        if (HE_QAT_STATUS_SUCCESS != status) {
            printf("Failed at binToBigNumber()\n");
            exit(1);
        }

        // Reset numbers to 0
        memset(bn_base_data_, 0, len_);
        memset(bn_mod_data_, 0, len_);
        memset(bn_exponent_data_, 0, len_);
        // Make sure variables are reset
        if (memcmp(bn_base_data_, bn_mod_data_, len_) ||
            memcmp(bn_base_data_, bn_exponent_data_, len_)) {
            PRINT_ERR("Pointers are not reset to zero!");
            exit(1);
        }

        // start = clock();
        start = high_resolution_clock::now();
        status = bigNumberToBin(bn_base_data_, bit_length, big_num_base);
        if (HE_QAT_STATUS_SUCCESS != status) {
            printf("bn_base_data_: failed at bignumbertobin()\n");
            exit(1);
        }
        status = bigNumberToBin(bn_mod_data_, bit_length, big_num_mod);
        if (HE_QAT_STATUS_SUCCESS != status) {
            printf("bn_base_data_: failed at bignumbertobin()\n");
            exit(1);
        }
        status =
            bigNumberToBin(bn_exponent_data_, bit_length, big_num_exponent);
        if (HE_QAT_STATUS_SUCCESS != status) {
            printf("bn_base_data_: failed at bignumbertobin()\n");
            exit(1);
        }
        // cvt_elapsed += (clock() - start);
        cvt_duration +=
            duration_cast<microseconds>(high_resolution_clock::now() - start);

	omp_set_num_threads(nthreads);

	// Perform BigNumber modular exponentiation on QAT
        start = high_resolution_clock::now();

#pragma omp parallel private(status)
{
	int thread_id = omp_get_thread_num();
        unsigned int buffer_id = thread_id;
        
	// Secure one of the distributed outstanding buffers
	status = acquire_bnModExp_buffer(&buffer_id);
        if (HE_QAT_STATUS_SUCCESS != status) {
            printf("Error: acquire_bnModExp_buffer()\n");
	    exit(1);
	}
#ifdef HE_QAT_DEBUG
	printf("Thread #%d HE QAT ACQUIRED BUFFER ID: %u\n",thread_id,buffer_id);	
#endif
	// Divide work among threads
	unsigned int worksize = BATCH_SIZE/nthreads;
	unsigned int begin = thread_id*worksize;
	unsigned int end = begin + worksize;

#ifdef HE_QAT_DEBUG
	printf("Thread #%d Begin: %u End: %u\n",thread_id,begin,end);
#endif

	// For local thread, schedule work execution
	for (unsigned int b = begin; b < end; b++)
            status = HE_QAT_bnModExp_MT(buffer_id, bn_remainder_data_ + thread_id*len_, 
		bn_base_data_, bn_exponent_data_, bn_mod_data_, bit_length);
	
#ifdef HE_QAT_DEBUG
	printf("Thread #%d Waiting\n",thread_id);
#endif

	// Wait for the request to complete
	release_bnModExp_buffer(buffer_id, BATCH_SIZE/nthreads);

#ifdef HE_QAT_DEBUG
	printf("Thread #%d Completed\n",thread_id);
#endif
} // pragma omp parallel

        stop = high_resolution_clock::now();
        auto qat_duration = duration_cast<microseconds>(stop - start);

        ssl_avg_time =
            (mod * ssl_avg_time + ((double)(ssl_duration.count()))) / (mod + 1);
        qat_avg_time = (mod * qat_avg_time +
                        ((double)(qat_duration.count())) / BATCH_SIZE) /
                       (mod + 1);
        avg_speed_up = (mod * avg_speed_up +
                        (ssl_duration.count() /
                         (double)(qat_duration.count() / BATCH_SIZE))) /
                       (mod + 1);
        // qat_elapsed = clock() - start;

        // printf("BigNumber data conversion overhead: %.1lfus.\n",
        //       (cvt_elapsed / (CLOCKS_PER_SEC / 1000000.0)));
        // printf("BigNumber modular exponentiation on QAT: %.1lfus.\n",
        //       (qat_elapsed / (CLOCKS_PER_SEC / 1000000.0)));
        // qat_elapsed += cvt_elapsed;
        printf("Request #%u\t", mod + 1);
        printf("Overhead: %.1luus", cvt_duration.count());
        printf("\tOpenSSL: %.1lfus", ssl_avg_time);
        printf("\tQAT: %.1lfus", qat_avg_time);
        printf("\tSpeed-up: %.1lfx", avg_speed_up);
        // qat_elapsed += cvt_elapsed;

        BIGNUM* qat_res = BN_new();
        BN_bin2bn(bn_remainder_data_, len_, qat_res);

        if (HE_QAT_STATUS_SUCCESS != status) {
            PRINT_ERR("\nQAT bnModExp with BigNumber failed\n");
        }
#ifdef _DESTINY_DEBUG_VERBOSE
        else {
            PRINT_DBG("\nQAT bnModExpOp finished\n");
        }
#endif

        // start = clock();
        BigNumber big_num((Ipp32u)0);
        status = binToBigNumber(big_num, bn_remainder_data_, bit_length);
        if (HE_QAT_STATUS_SUCCESS != status) {
            printf("bn_remainder_data_: Failed at bigNumberToBin()\n");
            exit(1);
        }
        // qat_elapsed += (clock() - start);
        // printf("BigNumber ModExp total time: %.1lfus.\n",
        //       (qat_elapsed / (CLOCKS_PER_SEC / 1000000.0)));

#ifdef _DESTINY_DEBUG_VERBOSE
        bn_str = BN_bn2hex(qat_res);
        printf("Bin: %s num_bytes(%d) num_bits(%d)\n", bn_str,
               BN_num_bytes(qat_res), BN_num_bits(qat_res));
#endif

#ifdef _DESTINY_DEBUG_VERBOSE
        int bit_len = 0;
        ippsRef_BN(NULL, &bit_len, NULL, BN(big_num));
        std::string str;
        big_num.num2hex(str);
        printf("BigNumber:  %s num_bytes: %d num_bits: %d\n", str.c_str(), len_,
               bit_len);
        printf(
            "---------------------################-----------------------\n");
#endif

        if (BN_cmp(qat_res, ssl_res) != 0)
            printf("\t** FAIL **\n");
        else
            printf("\t** PASS **\n");

        BN_free(bn_mod);
        BN_free(bn_base);
        BN_free(bn_exponent);
        BN_free(qat_res);
        BN_free(ssl_res);

        //	OPENSSL_free(bn_str);

        free(bn_mod_data_);
        free(bn_base_data_);
        free(bn_exponent_data_);
        free(bn_remainder_data_);

//	break;
    }

    // Tear down OpenSSL context
    BN_CTX_end(ctx);

    // Tear down QAT runtime context
    release_qat_devices();

    return (int)status;
}
