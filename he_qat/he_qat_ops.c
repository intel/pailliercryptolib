/// @file he_qat_ops.c

#include "cpa.h"
#include "cpa_cy_im.h"
#include "cpa_cy_ln.h"
#include "icp_sal_poll.h"

#include "cpa_sample_utils.h"

#include "he_qat_types.h"
#include "he_qat_bn_ops.h"

#ifdef HE_QAT_PERF
#include <sys/time.h>
#endif

#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <openssl/bn.h>

#ifdef HE_QAT_SYNC_MODE
#pragma message "Synchronous execution mode."
#else
#pragma message "Asynchronous execution mode."
#endif

#include "he_qat_gconst.h"

// Global buffer for the runtime environment
extern HE_QAT_RequestBuffer he_qat_buffer;
extern HE_QAT_OutstandingBuffer outstanding;

// Callback functions
extern void HE_QAT_BIGNUMModExpCallback(void* pCallbackTag, CpaStatus status, void* pOpData, CpaFlatBuffer* pOut);
extern void HE_QAT_bnModExpCallback(void* pCallbackTag, CpaStatus status, void* pOpData, CpaFlatBuffer* pOut);

/// @brief Thread-safe producer implementation for the shared request buffer.
/// @details Fill internal or outstanding buffer with incoming work requests. 
///          This function is implemented in he_qat_ctrl.c.
extern void submit_request(HE_QAT_RequestBuffer* _buffer, void* args);


/* 
 * **************************************************************************
 *  Implementation of Functions for the Single Interface Support
 * **************************************************************************
 */

HE_QAT_STATUS HE_QAT_bnModExp(unsigned char* r, unsigned char* b,
                              unsigned char* e, unsigned char* m, int nbits) {
    static unsigned long long req_count = 0;
    
    // Unpack data and copy to QAT friendly memory space
    int len = (nbits + 7) >> 3;  // nbits / 8;

    if (NULL == r) return HE_QAT_STATUS_INVALID_PARAM;
    if (NULL == b) return HE_QAT_STATUS_INVALID_PARAM;
    if (NULL == e) return HE_QAT_STATUS_INVALID_PARAM;
    if (NULL == m) return HE_QAT_STATUS_INVALID_PARAM;

    Cpa8U* pBase = NULL;
    Cpa8U* pModulus = NULL;
    Cpa8U* pExponent = NULL;

    // TODO(fdiasmor): Try it with 8-byte alignment.
    CpaStatus status = CPA_STATUS_FAIL;
    // status = PHYS_CONTIG_ALLOC(&pBase, len);
    status = PHYS_CONTIG_ALLOC_ALIGNED(&pBase, len, 8);
    if (CPA_STATUS_SUCCESS == status && NULL != pBase) {
        memcpy(pBase, b, len);
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
        return HE_QAT_STATUS_FAIL;
    }

    // status = PHYS_CONTIG_ALLOC(&pExponent, len);
    status = PHYS_CONTIG_ALLOC_ALIGNED(&pExponent, len, 8);
    if (CPA_STATUS_SUCCESS == status && NULL != pExponent) {
        memcpy(pExponent, e, len);
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
        return HE_QAT_STATUS_FAIL;
    }

    // status = PHYS_CONTIG_ALLOC(&pModulus, len);
    status = PHYS_CONTIG_ALLOC_ALIGNED(&pModulus, len, 8);
    if (CPA_STATUS_SUCCESS == status && NULL != pModulus) {
        memcpy(pModulus, m, len);
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
        return HE_QAT_STATUS_FAIL;
    }

    // HE_QAT_TaskRequest request =
    //           HE_QAT_PACK_MODEXP_REQUEST(pBase, pExponent, pModulus, r)
    // Pack it as a QAT Task Request
    HE_QAT_TaskRequest* request =
        (HE_QAT_TaskRequest*)calloc(1, sizeof(HE_QAT_TaskRequest));
    if (NULL == request) {
        printf(
            "HE_QAT_TaskRequest memory allocation failed in "
            "bnModExpPerformOp.\n");
        return HE_QAT_STATUS_FAIL;
    }

    CpaCyLnModExpOpData* op_data =
        (CpaCyLnModExpOpData*)calloc(1, sizeof(CpaCyLnModExpOpData));
    if (NULL == op_data) {
        printf("Cpa memory allocation failed in bnModExpPerformOp.\n");
        return HE_QAT_STATUS_FAIL;
    }
    op_data->base.pData = pBase;
    op_data->base.dataLenInBytes = len;
    op_data->exponent.pData = pExponent;
    op_data->exponent.dataLenInBytes = len;
    op_data->modulus.pData = pModulus;
    op_data->modulus.dataLenInBytes = len;
    request->op_data = (void*)op_data;

    // status = PHYS_CONTIG_ALLOC(&request->op_result.pData, len);
    status = PHYS_CONTIG_ALLOC_ALIGNED(&request->op_result.pData, len, 8);
    if (CPA_STATUS_SUCCESS == status && NULL != request->op_result.pData) {
        request->op_result.dataLenInBytes = len;
    } else {
        printf(
            "CpaFlatBuffer.pData memory allocation failed in "
            "bnModExpPerformOp.\n");
        return HE_QAT_STATUS_FAIL;
    }

    request->op_type = HE_QAT_OP_MODEXP;
    request->callback_func = (void*)HE_QAT_bnModExpCallback;
    request->op_status = status;
    request->op_output = (void*)r;
    
    request->id = req_count++;
    
    // Ensure calls are synchronized at exit (blocking)
    pthread_mutex_init(&request->mutex, NULL);
    pthread_cond_init(&request->ready, NULL);

#ifdef HE_QAT_DEBUG
    printf("BN ModExp interface call for request #%llu\n", req_count);
#endif

    // Submit request using producer function
    submit_request(&he_qat_buffer, (void*)request);

    return HE_QAT_STATUS_SUCCESS;
}

HE_QAT_STATUS HE_QAT_BIGNUMModExp(BIGNUM* r, BIGNUM* b, BIGNUM* e, BIGNUM* m, int nbits) {
    static unsigned long long req_count = 0;
    
    // Unpack data and copy to QAT friendly memory space
    int len = (nbits + 7) >> 3; 

    Cpa8U* pBase = NULL;
    Cpa8U* pModulus = NULL;
    Cpa8U* pExponent = NULL;

    HE_QAT_TaskRequest* request =
        (HE_QAT_TaskRequest*)calloc(1, sizeof(HE_QAT_TaskRequest));
    if (NULL == request) {
        printf(
            "HE_QAT_TaskRequest memory allocation failed in "
            "bnModExpPerformOp.\n");
        return HE_QAT_STATUS_FAIL;
    }

    // TODO: @fdiasmor Try it with 8-byte alignment.
    CpaStatus status = CPA_STATUS_FAIL;
    status = PHYS_CONTIG_ALLOC(&pBase, len);
    if (CPA_STATUS_SUCCESS == status && NULL != pBase) {
        if (!BN_bn2binpad(b, pBase, len)) {
            printf("BN_bn2binpad (base) failed in bnModExpPerformOp.\n");
            PHYS_CONTIG_FREE(pBase);
            return HE_QAT_STATUS_FAIL;
        }
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
        return HE_QAT_STATUS_FAIL;
    }

    status = PHYS_CONTIG_ALLOC(&pExponent, len);
    if (CPA_STATUS_SUCCESS == status && NULL != pExponent) {
        if (!BN_bn2binpad(e, pExponent, len)) {
            printf("BN_bn2binpad (exponent) failed in bnModExpPerformOp.\n");
            PHYS_CONTIG_FREE(pExponent);
            return HE_QAT_STATUS_FAIL;
        }
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
        return HE_QAT_STATUS_FAIL;
    }

    status = PHYS_CONTIG_ALLOC(&pModulus, len);
    if (CPA_STATUS_SUCCESS == status && NULL != pModulus) {
        if (!BN_bn2binpad(m, pModulus, len)) {
            printf("BN_bn2binpad failed in bnModExpPerformOp.\n");
            PHYS_CONTIG_FREE(pModulus);
            return HE_QAT_STATUS_FAIL;
        }
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
        return HE_QAT_STATUS_FAIL;
    }

    // Pack it as a QAT Task Request
    CpaCyLnModExpOpData* op_data =
        (CpaCyLnModExpOpData*)calloc(1, sizeof(CpaCyLnModExpOpData));
    if (NULL == op_data) {
        printf("Cpa memory allocation failed in bnModExpPerformOp.\n");
        return HE_QAT_STATUS_FAIL;
    }
    op_data->base.pData = pBase;
    op_data->base.dataLenInBytes = len;
    op_data->exponent.pData = pExponent;
    op_data->exponent.dataLenInBytes = len;
    op_data->modulus.pData = pModulus;
    op_data->modulus.dataLenInBytes = len;
    request->op_data = (void*)op_data;

    status = PHYS_CONTIG_ALLOC(&request->op_result.pData, len);
    if (CPA_STATUS_SUCCESS == status && NULL != request->op_result.pData) {
        request->op_result.dataLenInBytes = len;
    } else {
        printf(
            "CpaFlatBuffer.pData memory allocation failed in "
            "bnModExpPerformOp.\n");
        return HE_QAT_STATUS_FAIL;
    }

    request->op_type = HE_QAT_OP_MODEXP;
    request->callback_func = (void*)HE_QAT_BIGNUMModExpCallback;
    request->op_status = status;
    request->op_output = (void*)r;
    
    request->id = req_count++;
    // Ensure calls are synchronized at exit (blocking)
    pthread_mutex_init(&request->mutex, NULL);
    pthread_cond_init(&request->ready, NULL);

    // Submit request using producer function
    submit_request(&he_qat_buffer, (void*)request);

    return HE_QAT_STATUS_SUCCESS;
}

void getBnModExpRequest(unsigned int batch_size) {
    static unsigned long block_at_index = 0;
    unsigned int j = 0;

#ifdef HE_QAT_PERF
    struct timeval start_time, end_time;
    double time_taken = 0.0;
    gettimeofday(&start_time, NULL);
#endif
//    while (j < batch_size) {
    do {
        // Buffer read may be safe for single-threaded blocking calls only.
        // Note: Not tested on multithreaded environment.
        HE_QAT_TaskRequest* task =
            (HE_QAT_TaskRequest*)he_qat_buffer.data[block_at_index];

        if (NULL == task)
           continue;

        // Block and synchronize: Wait for the most recently offloaded request
        // to complete processing
        pthread_mutex_lock(
            &task->mutex);  // mutex only needed for the conditional variable
        while (HE_QAT_STATUS_READY != task->request_status)
            pthread_cond_wait(&task->ready, &task->mutex);

#ifdef HE_QAT_PERF
        time_taken = (task->end.tv_sec - task->start.tv_sec) * 1e6;
        time_taken =
            (time_taken + (task->end.tv_usec - task->start.tv_usec));  //*1e-6;
        printf("%u time: %.1lfus\n", j, time_taken);
#endif

        // Free up QAT temporary memory
        CpaCyLnModExpOpData* op_data = (CpaCyLnModExpOpData*)task->op_data;
        if (op_data) {
            PHYS_CONTIG_FREE(op_data->base.pData);
            PHYS_CONTIG_FREE(op_data->exponent.pData);
            PHYS_CONTIG_FREE(op_data->modulus.pData);
        }
        free(task->op_data);
        task->op_data = NULL;
        if (task->op_result.pData) {
            PHYS_CONTIG_FREE(task->op_result.pData);
        }

        // Move forward to wait for the next request that will be offloaded
        pthread_mutex_unlock(&task->mutex);

        // Fix segmentation fault?
        free(he_qat_buffer.data[block_at_index]);
        he_qat_buffer.data[block_at_index] = NULL;

        block_at_index = (block_at_index + 1) % HE_QAT_BUFFER_SIZE;
//        j++;
//    }
    } while (++j < batch_size); // number of null pointers equal batch size ?

#ifdef HE_QAT_PERF
    gettimeofday(&end_time, NULL);
    time_taken = (end_time.tv_sec - start_time.tv_sec) * 1e6;
    time_taken =
        (time_taken + (end_time.tv_usec - start_time.tv_usec));  //*1e-6;
    printf("Batch Wall Time: %.1lfus\n", time_taken);
#endif

    return;
}

/* 
 * **************************************************************************
 *  Implementation of Functions for the Multithreading Interface Support
 * **************************************************************************
 */

HE_QAT_STATUS HE_QAT_bnModExp_MT(unsigned int _buffer_id, unsigned char* r,
                                 unsigned char* b, unsigned char* e,
                                 unsigned char* m, int nbits) {
    static unsigned long long req_count = 0;
    
    // Unpack data and copy to QAT friendly memory space
    int len = (nbits + 7) >> 3;

    if (NULL == r) return HE_QAT_STATUS_INVALID_PARAM;
    if (NULL == b) return HE_QAT_STATUS_INVALID_PARAM;
    if (NULL == e) return HE_QAT_STATUS_INVALID_PARAM;
    if (NULL == m) return HE_QAT_STATUS_INVALID_PARAM;

    Cpa8U* pBase = NULL;
    Cpa8U* pModulus = NULL;
    Cpa8U* pExponent = NULL;

    // TODO(fdiasmor): Try it with 8-byte alignment.
    CpaStatus status = CPA_STATUS_FAIL;
    // status = PHYS_CONTIG_ALLOC(&pBase, len);
    status = PHYS_CONTIG_ALLOC_ALIGNED(&pBase, len, 8);
    if (CPA_STATUS_SUCCESS == status && NULL != pBase) {
        memcpy(pBase, b, len);
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
        return HE_QAT_STATUS_FAIL;
    }

    // status = PHYS_CONTIG_ALLOC(&pExponent, len);
    status = PHYS_CONTIG_ALLOC_ALIGNED(&pExponent, len, 8);
    if (CPA_STATUS_SUCCESS == status && NULL != pExponent) {
        memcpy(pExponent, e, len);
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
        return HE_QAT_STATUS_FAIL;
    }

    // status = PHYS_CONTIG_ALLOC(&pModulus, len);
    status = PHYS_CONTIG_ALLOC_ALIGNED(&pModulus, len, 8);
    if (CPA_STATUS_SUCCESS == status && NULL != pModulus) {
        memcpy(pModulus, m, len);
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
        return HE_QAT_STATUS_FAIL;
    }

    // HE_QAT_TaskRequest request =
    //           HE_QAT_PACK_MODEXP_REQUEST(pBase, pExponent, pModulus, r)
    // Pack it as a QAT Task Request
    HE_QAT_TaskRequest* request =
        (HE_QAT_TaskRequest*)calloc(1, sizeof(HE_QAT_TaskRequest));
    if (NULL == request) {
        printf(
            "HE_QAT_TaskRequest memory allocation failed in "
            "bnModExpPerformOp.\n");
        return HE_QAT_STATUS_FAIL;
    }

    CpaCyLnModExpOpData* op_data =
        (CpaCyLnModExpOpData*)calloc(1, sizeof(CpaCyLnModExpOpData));
    if (NULL == op_data) {
        printf("Cpa memory allocation failed in bnModExpPerformOp.\n");
        return HE_QAT_STATUS_FAIL;
    }
    op_data->base.pData = pBase;
    op_data->base.dataLenInBytes = len;
    op_data->exponent.pData = pExponent;
    op_data->exponent.dataLenInBytes = len;
    op_data->modulus.pData = pModulus;
    op_data->modulus.dataLenInBytes = len;
    request->op_data = (void*)op_data;

    // status = PHYS_CONTIG_ALLOC(&request->op_result.pData, len);
    status = PHYS_CONTIG_ALLOC_ALIGNED(&request->op_result.pData, len, 8);
    if (CPA_STATUS_SUCCESS == status && NULL != request->op_result.pData) {
        request->op_result.dataLenInBytes = len;
    } else {
        printf(
            "CpaFlatBuffer.pData memory allocation failed in "
            "bnModExpPerformOp.\n");
        return HE_QAT_STATUS_FAIL;
    }

    request->op_type = HE_QAT_OP_MODEXP;
    request->callback_func = (void*)HE_QAT_bnModExpCallback;
    request->op_status = status;
    request->op_output = (void*)r;

    request->id = req_count++;

    // Ensure calls are synchronized at exit (blocking)
    pthread_mutex_init(&request->mutex, NULL);
    pthread_cond_init(&request->ready, NULL);

#ifdef HE_QAT_DEBUG
    printf("BN ModExp interface call for request #%llu\n", req_count);
#endif

    // Submit request using producer function
    submit_request(&outstanding.buffer[_buffer_id], (void*)request);

    return HE_QAT_STATUS_SUCCESS;
}

/// @brief Frontend for multithreading support.
/// @details Try to acquire an available buffer to store outstanding work requests sent by caller.  
///          If none is available, it blocks further processing and waits until another caller's concurrent 
///          thread releases one. This function must be called before
///          calling HE_QAT_bnModExp_MT(.).
/// @param[out] Pointer to memory space allocated by caller to hold the buffer ID of the buffer used to store caller's outstanding requests.
HE_QAT_STATUS acquire_bnModExp_buffer(unsigned int* _buffer_id) {
    if (NULL == _buffer_id) return HE_QAT_STATUS_INVALID_PARAM;

    pthread_mutex_lock(&outstanding.mutex);

    // Wait until next outstanding buffer becomes available for use
    while (outstanding.busy_count >= HE_QAT_BUFFER_COUNT)
        pthread_cond_wait(&outstanding.any_free_buffer, &outstanding.mutex);

    assert(outstanding.busy_count < HE_QAT_BUFFER_COUNT);

    // Find next outstanding buffer available
    unsigned int next_free_buffer = outstanding.next_free_buffer;
    for (unsigned int i = 0; i < HE_QAT_BUFFER_COUNT; i++) {
        if (outstanding.free_buffer[next_free_buffer]) {
            outstanding.free_buffer[next_free_buffer] = 0;
            *_buffer_id = next_free_buffer;
            break;
        }
        next_free_buffer = (next_free_buffer + 1) % HE_QAT_BUFFER_COUNT;
    }

    outstanding.next_free_buffer = (*_buffer_id + 1) % HE_QAT_BUFFER_COUNT;
    outstanding.next_ready_buffer = *_buffer_id;
    outstanding.ready_buffer[*_buffer_id] = 1;
    outstanding.busy_count++;
    // busy meaning:
    // taken by a thread, enqueued requests, in processing, waiting results

    pthread_cond_signal(&outstanding.any_ready_buffer);
    pthread_mutex_unlock(&outstanding.mutex);

    return HE_QAT_STATUS_SUCCESS;
}

/// @brief Wait for request processing to complete and release previously acquired buffer. 
/// @details Caution: It assumes acquire_bnModExp_buffer(&_buffer_id) to be called first
/// to secure and be assigned an outstanding buffer for the target thread. 
/// Equivalent to getBnModExpRequests() for the multithreading support interfaces.
/// param[in] _buffer_id Buffer ID of the buffer to be released/unlock for reuse by the next concurrent thread.
/// param[in] _batch_size Total number of requests to wait for completion before releasing the buffer.
void release_bnModExp_buffer(unsigned int _buffer_id, unsigned int _batch_size) {
    unsigned int next_data_out = outstanding.buffer[_buffer_id].next_data_out;
    unsigned int j = 0;
    
#ifdef HE_QAT_DEBUG
    printf("release_bnModExp_buffer #%u\n", _buffer_id);
#endif

#ifdef HE_QAT_PERF
    struct timeval start_time, end_time;
    double time_taken = 0.0;
    gettimeofday(&start_time, NULL);
#endif

    while (j < _batch_size) {
        HE_QAT_TaskRequest* task =
            (HE_QAT_TaskRequest*)outstanding.buffer[_buffer_id]
                .data[next_data_out];

        if (NULL == task) continue;

#ifdef HE_QAT_DEBUG
        printf("BatchSize %u Buffer #%u Request #%u Waiting\n", _batch_size,
               _buffer_id, j);
#endif

        // Block and synchronize: Wait for the most recently offloaded request
        // to complete processing. Mutex only needed for the conditional variable.
        pthread_mutex_lock(&task->mutex); 
        while (HE_QAT_STATUS_READY != task->request_status)
            pthread_cond_wait(&task->ready, &task->mutex);

#ifdef HE_QAT_PERF
        time_taken = (task->end.tv_sec - task->start.tv_sec) * 1e6;
        time_taken =
            (time_taken + (task->end.tv_usec - task->start.tv_usec));  //*1e-6;
        printf("%u time: %.1lfus\n", j, time_taken);
#endif

        // Free up QAT temporary memory
        CpaCyLnModExpOpData* op_data = (CpaCyLnModExpOpData*)task->op_data;
        if (op_data) {
            PHYS_CONTIG_FREE(op_data->base.pData);
            PHYS_CONTIG_FREE(op_data->exponent.pData);
            PHYS_CONTIG_FREE(op_data->modulus.pData);
        }
        free(task->op_data);
        task->op_data = NULL;
        if (task->op_result.pData) {
            PHYS_CONTIG_FREE(task->op_result.pData);
        }

        // Move forward to wait for the next request that will be offloaded
        pthread_mutex_unlock(&task->mutex);

#ifdef HE_QAT_DEBUG
        printf("Buffer #%u Request #%u Completed\n", _buffer_id, j);
#endif
        // outstanding.buffer[_buffer_id].count--;

        free(outstanding.buffer[_buffer_id].data[next_data_out]);
        outstanding.buffer[_buffer_id].data[next_data_out] = NULL;

        // Update for next thread on the next external iteration
        next_data_out = (next_data_out + 1) % HE_QAT_BUFFER_SIZE;

        j++;
    }

#ifdef HE_QAT_PERF
    gettimeofday(&end_time, NULL);
    time_taken = (end_time.tv_sec - start_time.tv_sec) * 1e6;
    time_taken =
        (time_taken + (end_time.tv_usec - start_time.tv_usec));  //*1e-6;
    printf("Batch Wall Time: %.1lfus\n", time_taken);
#endif

    outstanding.buffer[_buffer_id].next_data_out = next_data_out;

    // Release outstanding buffer for usage by another thread
    pthread_mutex_lock(&outstanding.mutex);

    outstanding.next_free_buffer = _buffer_id;
    outstanding.ready_buffer[_buffer_id] = 0;
    outstanding.free_buffer[_buffer_id] = 1;
    outstanding.busy_count--;

    pthread_cond_signal(&outstanding.any_free_buffer);
    pthread_mutex_unlock(&outstanding.mutex);

    return;
}
