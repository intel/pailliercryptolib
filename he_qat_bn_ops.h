// New compilers
#pragma once

// Legacy compilers
#ifndef _HE_QAT_BN_OPS_H_
#define _HE_QAT_BN_OPS_H_

#include "cpa.h"
#include "cpa_cy_ln.h"

#include "he_qat_types.h"

#include <semaphore.h>
#include <openssl/bn.h>

// #include "bignum.h" or create a standard interface 

#define HE_QAT_MAX_RETRY 100

// One for each consumer
typedef struct {
    sem_t callback;
    HE_QAT_OP op_type;
    CpaStatus op_status;
    CpaFlatBuffer op_output;
    CpaCyLnModExpOpData op_data;
    //void *op_data;
    HE_QAT_STATUS request_status;
} HE_QAT_TaskRequest;


/// @brief
/// @function
/// Perform big number modular exponentiation for input data in 
/// OpenSSL's BIGNUM format.
/// @param[out] Remainder number of the modular exponentiation operation. 
/// @param[in] Base number of the modular exponentiation operation.
/// @param[in] Exponent number of the modular exponentiation operation.
/// @param[in] Modulus number of the modular exponentiation operation.
/// @param[in] Number of bits (bit precision) of input/output big numbers.
HE_QAT_STATUS bnModExpPerformOp(BIGNUM *r, BIGNUM *b, BIGNUM *e, BIGNUM *m, int nbits);

/// @brief
/// @function setModExpRequest(unsigned worksize)
/// Allocate buffer to hold and monitor up to 'worksize' number of outstanding requests.
/// @param[in] worksize Total number of modular exponentiation to perform in the next code section.
//void setModExpRequest(unsigned worksize)

/// @brief
/// @function getModExpRequest()
/// Monitor outstanding request to be complete and then deallocate buffer holding outstanding request; 
/// thus, releasing temporary memory.
// create private buffer for code section
// create QAT contiguous memory space
// copy data and package into a request
// use producer to place request into qat buffer
// wait for all outstanding requests to complete
//getModExpRequest()

#endif 
