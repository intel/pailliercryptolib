// New compilers
#pragma once

// Legacy compilers
#ifndef _HE_QAT_BN_OPS_H_
#define _HE_QAT_BN_OPS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cpa.h"
#include "cpa_cy_ln.h"

#include "he_qat_types.h"
#include "cpa_sample_utils.h"
#include <pthread.h>

//#include <semaphore.h>
#include <openssl/bn.h>

// #include "bignum.h" or create a standard interface

#define HE_QAT_MAX_RETRY 100

// One for each consumer
typedef struct {
    // sem_t callback;
    struct COMPLETION_STRUCT callback;
    HE_QAT_OP op_type;
    CpaStatus op_status;
    CpaFlatBuffer op_result;
    // CpaCyLnModExpOpData op_data;
    void* op_data;
    void* op_output;
    HE_QAT_STATUS request_status;
    pthread_mutex_t mutex;
    pthread_cond_t ready;
} HE_QAT_TaskRequest;

/// @brief
/// @function
/// Perform big number modular exponentiation for input data in
/// OpenSSL's BIGNUM format.
/// @details
/// Create private buffer for code section. Create QAT contiguous memory space.
/// Copy data and package into a request and call producer function to submit
/// request into qat buffer.
/// @param r    [out] Remainder number of the modular exponentiation operation.
/// @param b    [in] Base number of the modular exponentiation operation.
/// @param e    [in] Exponent number of the modular exponentiation operation.
/// @param m    [in] Modulus number of the modular exponentiation operation.
/// @param nbits[in] Number of bits (bit precision) of input/output big numbers.
HE_QAT_STATUS bnModExpPerformOp(BIGNUM* r, BIGNUM* b, BIGNUM* e, BIGNUM* m,
                                int nbits);

/// @brief
/// @function setModExpRequest(unsigned worksize)
/// Allocate buffer to hold and monitor up to 'worksize' number of outstanding
/// requests.
/// @param[in] worksize Total number of modular exponentiation to perform in the
/// next code section.
// void setModExpRequest(unsigned worksize)

/// @brief
/// @function getModExpRequest()
/// Monitor outstanding request to be complete and then deallocate buffer
/// holding outstanding request;
/// @details
/// Releasing QAT temporary memory.

// wait for all outstanding requests to complete
// void getBnModExpRequest();
void getBnModExpRequest(unsigned int num_requests);
/// thus, releasing temporary memory.
// create private buffer for code section
// create QAT contiguous memory space
// copy data and package into a request
// use producer to place request into qat buffer
// wait for all outstanding requests to complete

/// @brief
/// @function
/// Generic big number modular exponentiation for input data in primitive type format (unsigned char *).
/// @details
/// Create private buffer for code section. Create QAT contiguous memory space.
/// Copy data and package into a request and call producer function to submit
/// request into qat buffer.
/// @param[out] r Remainder number of the modular exponentiation operation.
/// @param[in] b Base number of the modular exponentiation operation.
/// @param[in] e Exponent number of the modular exponentiation operation.
/// @param[in] m Modulus number of the modular exponentiation operation.
/// @param[in] nbits Number of bits (bit precision) of input/output big numbers.
HE_QAT_STATUS HE_QAT_bnModExp(unsigned char* r, unsigned char* b, unsigned char* e, unsigned char* m, int nbits);

#ifdef __cplusplus
} //extern "C" {
#endif

#endif
