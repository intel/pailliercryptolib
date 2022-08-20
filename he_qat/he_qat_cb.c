// qatlib headers
#include <cpa.h>
#include <cpa_sample_utils.h>

// C library
#include <pthread.h>
#include <openssl/bn.h>

// local headers
#include "he_qat_types.h"

// Global variables
extern volatile unsigned long request_count; // = 0;
extern volatile unsigned long response_count; // = 0;
extern pthread_mutex_t response_mutex;

/// @brief
/// @function
/// Callback function for lnModExpPerformOp. It performs any data processing
/// required after the modular exponentiation.
void HE_QAT_BIGNUMModExpCallback(void* pCallbackTag,  
                             CpaStatus status,
                             void* pOpData,  
                             CpaFlatBuffer* pOut) {
    HE_QAT_TaskRequest* request = NULL;

    // Check if input data for the op is available and do something
    if (NULL != pCallbackTag) {
        // Read request data
        request = (HE_QAT_TaskRequest*)pCallbackTag;

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


/// @brief
/// @function
/// Callback function for HE_QAT_bnModExp. It performs any data processing
/// required after the modular exponentiation.
void HE_QAT_bnModExpCallback(
    void* pCallbackTag,  // This type can be variable
    CpaStatus status,
    void* pOpData,  // This is fixed -- please swap it
    CpaFlatBuffer* pOut) {
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
//		printf("Request ID %llu Completed.\n",request->id);
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
