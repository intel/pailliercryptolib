
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

#include <pthread.h>
#include <assert.h>

#ifdef HE_QAT_SYNC_MODE
#pragma message "Synchronous execution mode."
#else
#pragma message "Asynchronous execution mode."
#endif

// Global buffer for the runtime environment
HE_QAT_RequestBuffer he_qat_buffer;

/// @brief
/// @function
/// Callback function for lnModExpPerformOp. It performs any data processing
/// required after the modular exponentiation.
static void lnModExpCallback(void* pCallbackTag,  // This type can be variable
                             CpaStatus status,
                             void* pOpData,  // This is fixed -- please swap it
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
/// Thread-safe producer implementation for the shared request buffer.
/// Stores requests in a buffer that will be offload to QAT devices.
static void submit_request(HE_QAT_RequestBuffer* _buffer, void* args) {
#ifdef HE_QAT_DEBUG
    printf("Lock write request\n");
#endif
    pthread_mutex_lock(&_buffer->mutex);

#ifdef HE_QAT_DEBUG
    printf("Wait lock write request. [buffer size: %d]\n", _buffer->count);
#endif
    while (_buffer->count >= HE_QAT_BUFFER_SIZE)
        pthread_cond_wait(&_buffer->any_free_slot, &_buffer->mutex);

    assert(_buffer->count < HE_QAT_BUFFER_SIZE);

    _buffer->data[_buffer->next_free_slot++] = args;

    _buffer->next_free_slot %= HE_QAT_BUFFER_SIZE;
    _buffer->count++;

    pthread_cond_signal(&_buffer->any_more_data);
    pthread_mutex_unlock(&_buffer->mutex);
#ifdef HE_QAT_DEBUG
    printf("Unlocked write request. [buffer size: %d]\n", _buffer->count);
#endif
}

/// @brief
/// @function
/// Thread-safe consumer implementation for the shared request buffer.
/// Read requests from a buffer to finally offload the work to QAT devices.
static HE_QAT_TaskRequest* read_request(HE_QAT_RequestBuffer* _buffer) {
    void* item = NULL;
    pthread_mutex_lock(&_buffer->mutex);
    // Wait while buffer is empty
    while (_buffer->count <= 0)
        pthread_cond_wait(&_buffer->any_more_data, &_buffer->mutex);

    assert(_buffer->count > 0);

    item = _buffer->data[_buffer->next_data_slot++];

    // TODO(fdiasmor): for multithreading mode
    // Make copy of request so that the buffer can be reused

    _buffer->next_data_slot %= HE_QAT_BUFFER_SIZE;
    _buffer->count--;

    pthread_cond_signal(&_buffer->any_free_slot);
    pthread_mutex_unlock(&_buffer->mutex);

    return (HE_QAT_TaskRequest*)(item);
}

/// @brief
/// @function start_inst_polling
/// @param[in] HE_QAT_InstConfig Parameter values to start and poll instances.
///
static void* start_inst_polling(void* _inst_config) {
    if (NULL == _inst_config) {
        printf(
            "Failed at start_inst_polling: argument is NULL.");  //,__FUNC__);
        pthread_exit(NULL);
    }

    HE_QAT_InstConfig* config = (HE_QAT_InstConfig*)_inst_config;

    if (NULL == config->inst_handle) return NULL;

    // What is harmful for polling without performing any operation?
    config->polling = 1;
    while (config->polling) {
        icp_sal_CyPollInstance(config->inst_handle, 0);
        OS_SLEEP(50);
    }

    pthread_exit(NULL);
}

/// @brief
/// @function perform_op
/// Offload operation to QAT endpoints; for example, large number modular
/// exponentiation.
/// @param[in] HE_QAT_InstConfig *: contains the handle to CPA instance, pointer
/// the global buffer of requests.
void* start_perform_op(void* _inst_config) {
    if (NULL == _inst_config) {
        printf("Failed in start_perform_op: _inst_config is NULL.\n");
        pthread_exit(NULL);
    }

    HE_QAT_InstConfig* config = (HE_QAT_InstConfig*)_inst_config;

    CpaStatus status = CPA_STATUS_FAIL;

    // Start from zero or restart after stop_perform_op
    pthread_mutex_lock(&config->mutex);
    while (config->active) pthread_cond_wait(&config->ready, &config->mutex);

    // assert(0 == config->active);
    // assert(NULL == config->inst_handle);

    status = cpaCyStartInstance(config->inst_handle);
    config->status = status;
    if (CPA_STATUS_SUCCESS == status) {
        printf("Cpa CyInstance has successfully started.\n");
        status =
            cpaCySetAddressTranslation(config->inst_handle, sampleVirtToPhys);
    }

    pthread_cond_signal(&config->ready);
    pthread_mutex_unlock(&config->mutex);

    if (CPA_STATUS_SUCCESS != status) pthread_exit(NULL);

    // Start QAT instance and start polling
    pthread_t polling_thread;
    if (pthread_create(&polling_thread, config->attr, start_inst_polling,
                       (void*)config) != 0) {
        printf("Failed at creating and starting polling thread.\n");
        pthread_exit(NULL);
    }

    if (pthread_detach(polling_thread) != 0) {
        printf("Failed at detaching polling thread.\n");
        pthread_exit(NULL);
    }

    config->running = 1;
    config->active = 1;
    while (config->running) {
#ifdef HE_QAT_DEBUG
        printf("Try reading request from buffer. Inst #%d\n", config->inst_id);
#endif
        // Try consume data from butter to perform requested operation
        HE_QAT_TaskRequest* request =
            (HE_QAT_TaskRequest*)read_request(&he_qat_buffer);

        if (NULL == request) {
            pthread_cond_signal(&config->ready);
            continue;
        }
#ifdef HE_QAT_SYNC_MODE
        COMPLETION_INIT(&request->callback);
#endif
        unsigned retry = 0;
        do {
            // Realize the type of operation from data
            switch (request->op_type) {
            // Select appropriate action
            case HE_QAT_OP_MODEXP:
#ifdef HE_QAT_DEBUG
                printf("Offload request using instance #%d\n", config->inst_id);
#endif
#ifdef HE_QAT_PERF
                gettimeofday(&request->start, NULL);
#endif
                status = cpaCyLnModExp(
                    config->inst_handle,
                    (CpaCyGenFlatBufCbFunc)
                        request->callback_func,  // lnModExpCallback,
                    (void*)request, (CpaCyLnModExpOpData*)request->op_data,
                    &request->op_result);
                retry++;
                break;
            case HE_QAT_OP_NONE:
            default:
#ifdef HE_QAT_DEBUG
                printf("HE_QAT_OP_NONE to instance #%d\n", config->inst_id);
#endif
                retry = HE_QAT_MAX_RETRY;
                break;
            }
        } while (CPA_STATUS_RETRY == status && retry < HE_QAT_MAX_RETRY);

        // Ensure every call to perform operation is blocking for each endpoint
        if (CPA_STATUS_SUCCESS == status) {
//		printf("retry_count = %d\n",retry_count);
#ifdef HE_QAT_SYNC_MODE
            // Wait until the callback function has been called
            if (!COMPLETION_WAIT(&request->callback, TIMEOUT_MS)) {
                request->op_status = CPA_STATUS_FAIL;
                request->request_status = HE_QAT_STATUS_FAIL;  // Review it
                printf("Failed in COMPLETION WAIT\n");
            }

            // Destroy synchronization object
            COMPLETION_DESTROY(&request->callback);
#endif
        } else {
            request->op_status = CPA_STATUS_FAIL;
            request->request_status = HE_QAT_STATUS_FAIL;  // Review it
        }

        // Wake up any blocked call to stop_perform_op, signaling that now it is
        // safe to terminate running instances. Check if this detereorate
        // performance.
        pthread_cond_signal(
            &config->ready);  // Prone to the lost wake-up problem
#ifdef HE_QAT_DEBUG
        printf("Offloading completed by instance #%d\n", config->inst_id);
#endif
    }
    pthread_exit(NULL);
}

/// @brief
/// @function
/// Stop first 'num_inst' number of cpaCyInstance(s), including their polling
/// and running threads.
/// @param[in] HE_QAT_InstConfig config Vector of created instances with their
/// configuration setup.
/// @param[in] num_inst Unsigned integer number indicating first number of
/// instances to be terminated.
void stop_perform_op(HE_QAT_InstConfig* config, unsigned num_inst) {
    // if () {
    // Stop runnning and polling instances
    // Release QAT instances handles
    if (NULL == config) return;

    CpaStatus status = CPA_STATUS_FAIL;
    for (unsigned i = 0; i < num_inst; i++) {
        pthread_mutex_lock(&config[i].mutex);
#ifdef HE_QAT_DEBUG
        printf("Try teardown HE QAT instance #%d.\n", i);
#endif
        while (0 == config[i].active) {
            pthread_cond_wait(&config[i].ready, &config[i].mutex);
        }
        if (CPA_STATUS_SUCCESS == config[i].status && config[i].active) {
#ifdef HE_QAT_DEBUG
            printf("Stop polling and running threads #%d\n", i);
#endif
            config[i].polling = 0;
            config[i].running = 0;
            OS_SLEEP(10);
#ifdef HE_QAT_DEBUG
            printf("Stop cpaCyInstance #%d\n", i);
#endif
            if (config[i].inst_handle == NULL) continue;
#ifdef HE_QAT_DEBUG
            printf("cpaCyStopInstance\n");
#endif
            status = cpaCyStopInstance(config[i].inst_handle);
            if (CPA_STATUS_SUCCESS != status) {
                printf("Failed to stop QAT instance #%d\n", i);
            }
        }
        pthread_cond_signal(&config[i].ready);
        pthread_mutex_unlock(&config[i].mutex);
    }
    //}

    return;
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
    request->callback_func = (void*)lnModExpCallback;
    request->op_status = status;
    request->op_output = (void*)r;

    // Ensure calls are synchronized at exit (blocking)
    pthread_mutex_init(&request->mutex, NULL);
    pthread_cond_init(&request->ready, NULL);

    // Submit request using producer function
    submit_request(&he_qat_buffer, (void*)request);

    return HE_QAT_STATUS_SUCCESS;
}

// Maybe it will be useful to pass the number of requests to retrieve
// Pass post-processing function as argument to bring output to expected type
void getBnModExpRequest(unsigned int batch_size) {
    static unsigned long block_at_index = 0;
    unsigned int j = 0;

#ifdef HE_QAT_PERF
    gettimeofday(&start_time, NULL);
#endif
    do {
        // Buffer read may be safe for single-threaded blocking calls only.
        // Note: Not tested on multithreaded environment.
        HE_QAT_TaskRequest* task =
            (HE_QAT_TaskRequest*)he_qat_buffer.data[block_at_index];

        // if (NULL == task)
        //   continue;

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
    } while (++j < batch_size);

#ifdef HE_QAT_PERF
    gettimeofday(&end_time, NULL);
    time_taken = (end_time.tv_sec - start_time.tv_sec) * 1e6;
    time_taken =
        (time_taken + (end_time.tv_usec - start_time.tv_usec));  //*1e-6;
    printf("Batch Wall Time: %.1lfus\n", time_taken);
#endif

    return;
}

/// @brief
/// @function
/// Callback function for HE_QAT_bnModExp. It performs any data processing
/// required after the modular exponentiation.
static void HE_QAT_bnModExpCallback(
    void* pCallbackTag,  // This type can be variable
    CpaStatus status,
    void* pOpData,  // This is fixed -- please swap it
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

HE_QAT_STATUS HE_QAT_bnModExp(unsigned char* r, unsigned char* b,
                              unsigned char* e, unsigned char* m, int nbits) {
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
    submit_request(&he_qat_buffer, (void*)request);

    return HE_QAT_STATUS_SUCCESS;
}
