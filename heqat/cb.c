// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
/// @file heqat/cb.c

// QAT-API headers
#include <cpa.h>

// C support libraries
#include <pthread.h>
#include <string.h>
#include <openssl/bn.h>

// Local headers
#include "heqat/common/types.h"
#include "heqat/common/utils.h"

// Global variables
static pthread_mutex_t response_mutex; ///< It protects against race condition on response_count due to concurrent callback events.
extern volatile unsigned long response_count; ///< It counts the number of requests completed by the accelerator.

/// @brief Callback implementation for the API HE_QAT_BIGNUMModExp(...) 
/// Callback function for the interface HE_QAT_BIGNUMModExp(). It performs 
/// any data post-processing required after the modular exponentiation.
/// @param[in] pCallbackTag work request package containing the original input data and other resources for post-processing.
/// @param[in] status CPA_STATUS of the performed operation, e.g. CyLnModExp().
/// @param[in] pOpData original input data passed to accelerator to perform the target operation (cannot be NULL).
/// @param[out] pOut output returned by the accelerator after executing the target operation.
void HE_QAT_BIGNUMModExpCallback(void* pCallbackTag, CpaStatus status, void* pOpData, CpaFlatBuffer* pOut) 
{
    HE_QAT_TaskRequest* request = NULL;

    // Check if input data for the op is available and do something
    if (NULL != pCallbackTag) {
        // Read request data
        request = (HE_QAT_TaskRequest*)pCallbackTag;
    	
	    pthread_mutex_lock(&response_mutex);
        // Global track of reponses by accelerator
        response_count += 1;
        pthread_mutex_unlock(&response_mutex);

        pthread_mutex_lock(&request->mutex);
        // Collect the device output in pOut
        request->op_status = status;
        if (CPA_STATUS_SUCCESS == status) {
            if (pOpData == request->op_data) {
                // Mark request as complete or ready to be used
                request->request_status = HE_QAT_STATUS_READY;

                BIGNUM* r = BN_bin2bn(request->op_result.pData,
                                      request->op_result.dataLenInBytes,
                                      (BIGNUM*)request->op_output);
		        if (NULL == r) 
                   request->request_status = HE_QAT_STATUS_FAIL;
#ifdef HE_QAT_PERF
                gettimeofday(&request->end, NULL);
#endif		   
            } else {
                request->request_status = HE_QAT_STATUS_FAIL;
            }
        }
        // Make it synchronous and blocking
        pthread_cond_signal(&request->ready);
        pthread_mutex_unlock(&request->mutex);
#ifdef HE_QAT_SYNC_MODE
        COMPLETE((struct COMPLETION_STRUCT*)&request->callback);
#endif
    }

    return;
}

/// @brief Callback implementation for the API HE_QAT_bnModExp(...) 
/// Callback function for the interface HE_QAT_bnModExp(). It performs 
/// any data post-processing required after the modular exponentiation.
/// @param[in] pCallbackTag work request package containing the original input data and other resources for post-processing.
/// @param[in] status CPA_STATUS of the performed operation, e.g. CyLnModExp().
/// @param[in] pOpData original input data passed to accelerator to perform the target operation (cannot be NULL).
/// @param[out] pOut output returned by the accelerator after executing the target operation.
void HE_QAT_bnModExpCallback(void* pCallbackTag, CpaStatus status, void* pOpData, CpaFlatBuffer* pOut) 
{
    HE_QAT_TaskRequest* request = NULL;
 
    // Check if input data for the op is available and do something
    if (NULL != pCallbackTag) {
        // Read request data
        request = (HE_QAT_TaskRequest*)pCallbackTag;
    
    	pthread_mutex_lock(&response_mutex);
        // Global track of reponses by accelerator
        response_count += 1;
        pthread_mutex_unlock(&response_mutex);

        pthread_mutex_lock(&request->mutex);
        // Collect the device output in pOut
        request->op_status = status;
        if (CPA_STATUS_SUCCESS == status) {
            if (pOpData == request->op_data) {
                // Mark request as complete or ready to be used
                request->request_status = HE_QAT_STATUS_READY;
                // Copy compute results to output destination
                memcpy(request->op_output, request->op_result.pData,
                       request->op_result.dataLenInBytes);
#ifdef HE_QAT_PERF
                gettimeofday(&request->end, NULL);
#endif
            } else {
                request->request_status = HE_QAT_STATUS_FAIL;
            }
        }
        // Make it synchronous and blocking
        pthread_cond_signal(&request->ready);
        pthread_mutex_unlock(&request->mutex);
#ifdef HE_QAT_SYNC_MODE
        COMPLETE((struct COMPLETION_STRUCT*)&request->callback);
#endif
    }

    return;
}
