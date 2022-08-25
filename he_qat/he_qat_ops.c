
#include "cpa.h"
#include "cpa_cy_im.h"
#include "cpa_cy_ln.h"
#include "icp_sal_poll.h"

#include "cpa_sample_utils.h"

#include "he_qat_types.h"
#include "he_qat_bn_ops.h"

#ifdef HE_QAT_PERF
#include <sys/time.h>
struct timeval start_time, end_time;
double time_taken = 0.0;
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

//#define RESTART_LATENCY_MICROSEC 600
//#define NUM_PKE_SLICES 6

#include "he_qat_gconst.h"

// Global buffer for the runtime environment
extern HE_QAT_RequestBuffer he_qat_buffer;
extern HE_QAT_OutstandingBuffer outstanding;

//volatile unsigned long response_count = 0;
//static volatile unsigned long request_count = 0;
//static unsigned long request_latency = 0; // unused
//static unsigned long restart_threshold = NUM_PKE_SLICES;//48; 
//static unsigned long max_pending = (NUM_PKE_SLICES * 2 * HE_QAT_NUM_ACTIVE_INSTANCES); 

// Callback functions
extern void HE_QAT_BIGNUMModExpCallback(void* pCallbackTag, CpaStatus status, void* pOpData, CpaFlatBuffer* pOut);
extern void HE_QAT_bnModExpCallback(void* pCallbackTag, CpaStatus status, void* pOpData, CpaFlatBuffer* pOut);


/// @brief
/// @function
/// Thread-safe producer implementation for the shared request buffer.
/// Stores requests in a buffer that will be offload to QAT devices.
extern void submit_request(HE_QAT_RequestBuffer* _buffer, void* args);

// Frontend for multithreading support
HE_QAT_STATUS acquire_bnModExp_buffer(unsigned int* _buffer_id) {
    if (NULL == _buffer_id) return HE_QAT_STATUS_INVALID_PARAM;

    pthread_mutex_lock(&outstanding.mutex);
    // Wait until next outstanding buffer becomes available for use
    while (outstanding.busy_count >= HE_QAT_BUFFER_COUNT)
        pthread_cond_wait(&outstanding.any_free_buffer, &outstanding.mutex);

    assert(outstanding.busy_count < HE_QAT_BUFFER_COUNT);

    // find next available
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

HE_QAT_STATUS bnModExpPerformOp(BIGNUM* r, BIGNUM* b, BIGNUM* e, BIGNUM* m,
                                int nbits) {
    // Unpack data and copy to QAT friendly memory space
    int len = (nbits + 7) >> 3;  // nbits / 8;

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
        //{
        //    unsigned char* bin = (unsigned char*)calloc(len, sizeof(unsigned
        //    char)); if (BN_bn2binpad(b, bin, len)) {
        if (BN_bn2binpad(b, pBase, len)) {
            //            memcpy(pBase, bin, len);
            // pBase = (Cpa8U*)bin;
        } else {
            printf("BN_bn2binpad (base) failed in bnModExpPerformOp.\n");
            PHYS_CONTIG_FREE(pBase);
            // free(bin);
            // bin = NULL;
            return HE_QAT_STATUS_FAIL;
        }
        //}
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
        return HE_QAT_STATUS_FAIL;
    }

    status = PHYS_CONTIG_ALLOC(&pExponent, len);
    if (CPA_STATUS_SUCCESS == status && NULL != pExponent) {
        //{
        //    unsigned char* bin = (unsigned char*)calloc(len, sizeof(unsigned
        //    char)); if (BN_bn2binpad(e, bin, len)) {
        if (BN_bn2binpad(e, pExponent, len)) {
            //            memcpy(pExponent, bin, len);
            // pExponent = (Cpa8U*)bin;
        } else {
            printf("BN_bn2binpad (exponent) failed in bnModExpPerformOp.\n");
            PHYS_CONTIG_FREE(pExponent);
            // free(bin);
            // bin = NULL;
            return HE_QAT_STATUS_FAIL;
        }
        //}
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
        return HE_QAT_STATUS_FAIL;
    }

    status = PHYS_CONTIG_ALLOC(&pModulus, len);
    if (CPA_STATUS_SUCCESS == status && NULL != pModulus) {
        //{
        //    unsigned char* bin = (unsigned char*)calloc(len, sizeof(unsigned
        //    char)); if (BN_bn2binpad(m, bin, len)) {
        if (BN_bn2binpad(m, pModulus, len)) {
            //            memcpy(pModulus, bin, len);
            // pModulus = (Cpa8U*)bin;
        } else {
            printf("BN_bn2binpad failed in bnModExpPerformOp.\n");
            PHYS_CONTIG_FREE(pModulus);
            // free(bin);
            // bin = NULL;
            return HE_QAT_STATUS_FAIL;
        }
        //}
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
    //request->callback_func = (void*)lnModExpCallback;
    request->callback_func = (void*)HE_QAT_BIGNUMModExpCallback;
    request->op_status = status;
    request->op_output = (void*)r;

    // Ensure calls are synchronized at exit (blocking)
    pthread_mutex_init(&request->mutex, NULL);
    pthread_cond_init(&request->ready, NULL);

    // Submit request using producer function
    submit_request(&he_qat_buffer, (void*)request);

    return HE_QAT_STATUS_SUCCESS;
}

// Assume allocate_bnModExp_buffer(&_buffer_id) to be called first
// to secure and allocate an outstanding buffer for the target thread.
// Multithread support for release_bnModExp_buffer(_buffer_id, batch_size)
void release_bnModExp_buffer(unsigned int _buffer_id,
                             unsigned int _batch_size) {
    unsigned int next_data_out = outstanding.buffer[_buffer_id].next_data_out;
    unsigned int j = 0;

#ifdef HE_QAT_DEBUG
    printf("release_bnModExp_buffer #%u\n", _buffer_id);
#endif

#ifdef HE_QAT_PERF
    gettimeofday(&start_time, NULL);
#endif

    while (j < _batch_size) {
        // Buffer read may be safe for single-threaded blocking calls only.
        // Note: Not tested on multithreaded environment.
        HE_QAT_TaskRequest* task =
            (HE_QAT_TaskRequest*)outstanding.buffer[_buffer_id]
                .data[next_data_out];

        if (NULL == task) continue;

#ifdef HE_QAT_DEBUG
        printf("BatchSize %u Buffer #%u Request #%u Waiting\n", _batch_size,
               _buffer_id, j);
#endif

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

#ifdef HE_QAT_DEBUG
        printf("Buffer #%u Request #%u Completed\n", _buffer_id, j);
#endif
        // outstanding.buffer[_buffer_id].count--;

        // Fix segmentation fault?
        free(outstanding.buffer[_buffer_id].data[next_data_out]);
        outstanding.buffer[_buffer_id].data[next_data_out] = NULL;

        // update for next thread on the next external iteration
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

// Maybe it will be useful to pass the number of requests to retrieve
// Pass post-processing function as argument to bring output to expected type
void getBnModExpRequest(unsigned int batch_size) {
    static unsigned long block_at_index = 0;
    unsigned int j = 0;

#ifdef HE_QAT_PERF
    gettimeofday(&start_time, NULL);
#endif
//printf("batch_size=%u \n",batch_size);
//    while (j < batch_size) {
    do {
        // Buffer read may be safe for single-threaded blocking calls only.
        // Note: Not tested on multithreaded environment.
        HE_QAT_TaskRequest* task =
            (HE_QAT_TaskRequest*)he_qat_buffer.data[block_at_index];

        if (NULL == task)
           continue;

//printf("j=%u ",j);

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

//    printf("\n");

    return;
}


HE_QAT_STATUS HE_QAT_bnModExp(unsigned char* r, unsigned char* b,
                              unsigned char* e, unsigned char* m, int nbits) {
//#ifdef HE_QAT_DEBUG
    static unsigned long long req_count = 0;
//#endif
    // Unpack data and copy to QAT friendly memory space
    //int len = nbits / 8;
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
    
    // Ensure calls are synchronized at exit (blocking)
    pthread_mutex_init(&request->mutex, NULL);
    pthread_cond_init(&request->ready, NULL);

    request->id = req_count;
#ifdef HE_QAT_DEBUG
    printf("BN ModExp interface call for request #%llu\n", ++req_count);
#endif

    // Submit request using producer function
    submit_request(&he_qat_buffer, (void*)request);

    return HE_QAT_STATUS_SUCCESS;
}

HE_QAT_STATUS HE_QAT_bnModExp_MT(unsigned int _buffer_id, unsigned char* r,
                                 unsigned char* b, unsigned char* e,
                                 unsigned char* m, int nbits) {
#ifdef HE_QAT_DEBUG
    static unsigned long long req_count = 0;
    // printf("Wait lock write request. [buffer size: %d]\n",_buffer->count);
#endif
    // Unpack data and copy to QAT friendly memory space
    int len = nbits / 8;

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

    // Ensure calls are synchronized at exit (blocking)
    pthread_mutex_init(&request->mutex, NULL);
    pthread_cond_init(&request->ready, NULL);

#ifdef HE_QAT_DEBUG
    printf("BN ModExp interface call for request #%llu\n", ++req_count);
#endif

    // Submit request using producer function
    submit_request(&outstanding.buffer[_buffer_id], (void*)request);

    return HE_QAT_STATUS_SUCCESS;
}
