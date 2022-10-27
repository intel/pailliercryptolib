/// @file heqat/ctrl.c

// QAT-API headers
#include "cpa.h"
#include "cpa_cy_im.h"
#include "cpa_cy_ln.h"
#include "icp_sal_poll.h"

// Global variables used to hold measured performance numbers.
#ifdef HE_QAT_PERF
#include <sys/time.h>
struct timeval start_time, end_time;
double time_taken = 0.0;
#endif

// C support libraries
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <openssl/bn.h>

// Local headers
#include "heqat/common/cpa_sample_utils.h"
#include "heqat/common/consts.h"
#include "heqat/common/types.h"
#include "heqat/bnops.h"

// Warn user on selected execution mode
#ifdef HE_QAT_SYNC_MODE
#pragma message "Synchronous execution mode."
#else
#pragma message "Asynchronous execution mode."
#endif

// Global buffer for the runtime environment
HE_QAT_RequestBuffer     he_qat_buffer; ///< This the internal buffer that holds and serializes the requests to the accelerator.
HE_QAT_OutstandingBuffer outstanding; ///< This is the data structure that holds outstanding requests from separate active threads calling the API.
volatile unsigned long response_count = 0; ///< Counter of processed requests and it is used to help control throttling.
static volatile unsigned long request_count = 0; ///< Counter of received requests and it is used to help control throttling.
static unsigned long restart_threshold = NUM_PKE_SLICES * HE_QAT_NUM_ACTIVE_INSTANCES; ///< Number of concurrent requests allowed to be sent to accelerator at once. 
static unsigned long max_pending = (2 * NUM_PKE_SLICES * HE_QAT_NUM_ACTIVE_INSTANCES); ///< Number of requests sent to the accelerator that are pending completion.

/// @brief Populate internal buffer with incoming requests from API calls.
/// @details This function is called from the main APIs to submit requests to 
/// a shared internal buffer for processing on QAT. It is a thread-safe implementation 
/// of the producer for the either the internal buffer or the outstanding buffer to 
/// host incoming requests. Depending on the buffer type, the submitted request 
/// is either ready to be scheduled or to be processed by the accelerator.
/// @param[out] _buffer Either `he_qat_buffer` or `outstanding` buffer.
/// @param[in] args Work request packaged in a custom data structure.
void submit_request(HE_QAT_RequestBuffer* _buffer, void* args) {
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

/// @brief Populates internal buffer with a list of work request.
/// @details This function is called by the request scheduler thread. It is a thread-safe implementation of the producer for the shared internal request buffer. This buffer stores and serializes the offloading of requests that are ready to be processed by the accelerator.
/// @param[out] _buffer reference pointer to the internal buffer `he_qat_buffer`.
/// @param[in] _requests list of requests retrieved from the buffer (`outstanding`) holding outstanding requests.
static void submit_request_list(HE_QAT_RequestBuffer* _buffer,
                                HE_QAT_TaskRequestList* _requests) {
#ifdef HE_QAT_DEBUG
    printf("Lock submit request list\n");
#endif
    if (0 == _requests->count) return;

    pthread_mutex_lock(&_buffer->mutex);

#ifdef HE_QAT_DEBUG
    printf(
        "Wait lock submit request list. [internal buffer size: %d] [num "
        "requests: %u]\n",
        _buffer->count, _requests->count);
#endif

    // Wait until buffer can accomodate the number of input requests
    while (_buffer->count >= HE_QAT_BUFFER_SIZE ||
           (HE_QAT_BUFFER_SIZE - _buffer->count) < _requests->count)
        pthread_cond_wait(&_buffer->any_free_slot, &_buffer->mutex);

    assert(_buffer->count < HE_QAT_BUFFER_SIZE);
    assert(_requests->count <= (HE_QAT_BUFFER_SIZE - _buffer->count));

    for (unsigned int i = 0; i < _requests->count; i++) {
        _buffer->data[_buffer->next_free_slot++] = _requests->request[i];
        _buffer->next_free_slot %= HE_QAT_BUFFER_SIZE;
        _requests->request[i] = NULL;
    }
    _buffer->count += _requests->count;
    _requests->count = 0;

    pthread_cond_signal(&_buffer->any_more_data);
    pthread_mutex_unlock(&_buffer->mutex);
#ifdef HE_QAT_DEBUG
    printf("Unlocked submit request list. [internal buffer size: %d]\n",
           _buffer->count);
#endif
}

/// @brief Retrieve multiple requests from the outstanding buffer.
/// @details Thread-safe consumer implementation for the outstanding request buffer.
/// Read requests from outstanding buffer (requests ready to be scheduled) to later pass 
/// them to the internal buffer `he_qat_buffer`. As those requests move from the outstanding 
/// buffer into the internal buffer, their state changes from ready-to-be-scheduled to 
/// ready-to-be-processed. This function is supported both in single-threaded or multi-threaded mode.
/// @param[out] _requests list of requests retrieved from internal buffer.
/// @param[in] _buffer buffer of type HE_QAT_RequestBuffer, typically the internal buffer in current implementation.
/// @param[in] max_requests maximum number of requests to retrieve from internal buffer, if available.
static void read_request_list(HE_QAT_TaskRequestList* _requests,
                              HE_QAT_RequestBuffer* _buffer, unsigned int max_requests) {
    if (NULL == _requests) return;

    pthread_mutex_lock(&_buffer->mutex);

    // Wait while buffer is empty
    while (_buffer->count <= 0)
        pthread_cond_wait(&_buffer->any_more_data, &_buffer->mutex);

    assert(_buffer->count > 0);
    // assert(_buffer->count <= HE_QAT_BUFFER_SIZE);

    unsigned int count = (_buffer->count < max_requests) ? _buffer->count : max_requests;

    //for (unsigned int i = 0; i < _buffer->count; i++) {
    for (unsigned int i = 0; i < count; i++) {
        _requests->request[i] = _buffer->data[_buffer->next_data_slot++];
        _buffer->next_data_slot %= HE_QAT_BUFFER_SIZE;
    }
    //_requests->count = _buffer->count;
    //_buffer->count = 0;
    _requests->count = count;
    _buffer->count -= count;

    pthread_cond_signal(&_buffer->any_free_slot);
    pthread_mutex_unlock(&_buffer->mutex);

    return;
}

/// @brief Read requests from the outstanding buffer.
/// @details Thread-safe consumer implementation for the outstanding request buffer.
/// Retrieve work requests from outstanding buffer (requests ready to be scheduled) to later on pass them to the internal buffer `he_qat_buffer`. As those requests move from the outstanding buffer into the internal buffer, their state changes from ready-to-be-scheduled to ready-to-be-processed.
/// This function is supported in single-threaded or multi-threaded mode.
/// @param[out] _requests list of work requests retrieved from outstanding buffer.
/// @param[in] _outstanding_buffer outstanding buffer holding requests in ready-to-be-scheduled state.
/// @param[in] max_requests maximum number of requests to retrieve from outstanding buffer if available.
static void pull_outstanding_requests(HE_QAT_TaskRequestList* _requests, HE_QAT_OutstandingBuffer* _outstanding_buffer, unsigned int max_num_requests) 
{
    if (NULL == _requests) return;
    _requests->count = 0;

    // for now, only one thread can change next_ready_buffer
    // so no need for sync tools

    // Select an outstanding buffer to pull requests and add them into the
    // processing queue (internal buffer)
    pthread_mutex_lock(&_outstanding_buffer->mutex);
    // Wait until next outstanding buffer becomes available for use
    while (outstanding.busy_count <= 0)
        pthread_cond_wait(&_outstanding_buffer->any_ready_buffer,
                          &_outstanding_buffer->mutex);

    int any_ready = 0;
    unsigned int index = _outstanding_buffer->next_ready_buffer;  // no fairness
    for (unsigned int i = 0; i < HE_QAT_BUFFER_COUNT; i++) {
        index = i;  // ensure fairness
        if (_outstanding_buffer->ready_buffer[index] &&
            _outstanding_buffer->buffer[index]
                .count) {  // sync with mutex at interface
            any_ready = 1;
            break;
        }
        // index = (index + 1) % HE_QAT_BUFFER_COUNT;
    }
    // Ensures it gets picked once only
    pthread_mutex_unlock(&_outstanding_buffer->mutex);

    if (!any_ready) return;

    // Extract outstanding requests from outstanding buffer
    // (this is the only function that reads from outstanding buffer, 
    // from a single thread)
    pthread_mutex_lock(&_outstanding_buffer->buffer[index].mutex);
    
    // This conditional waiting may not be required
    // Wait while buffer is empty
    while (_outstanding_buffer->buffer[index].count <= 0) {
        pthread_cond_wait(&_outstanding_buffer->buffer[index].any_more_data,
                          &_outstanding_buffer->buffer[index].mutex);
    }
    assert(_outstanding_buffer->buffer[index].count > 0);

    unsigned int num_requests =
        (_outstanding_buffer->buffer[index].count < max_num_requests)
            ? _outstanding_buffer->buffer[index].count
            : max_num_requests;

    assert(num_requests <= HE_QAT_BUFFER_SIZE);

    for (unsigned int i = 0; i < num_requests; i++) {
        _requests->request[i] =
            _outstanding_buffer->buffer[index]
                .data[_outstanding_buffer->buffer[index].next_data_slot];
        _outstanding_buffer->buffer[index].count--;
        _outstanding_buffer->buffer[index].next_data_slot++;
        _outstanding_buffer->buffer[index].next_data_slot %= HE_QAT_BUFFER_SIZE;
    }
    _requests->count = num_requests;

    pthread_cond_signal(&_outstanding_buffer->buffer[index].any_free_slot);
    pthread_mutex_unlock(&_outstanding_buffer->buffer[index].mutex);

    // ---------------------------------------------------------------------------
    // Notify there is an outstanding buffer in ready for the processing queue
    //    pthread_mutex_lock(&_outstanding_buffer->mutex);
    //
    //    _outstanding_buffer->ready_count--;
    //    _outstanding_buffer->ready_buffer[index] = 0;
    //
    //    pthread_cond_signal(&_outstanding_buffer->any_free_buffer);
    //    pthread_mutex_unlock(&_outstanding_buffer->mutex);

    return;
}

/// @brief Schedule outstanding requests to the internal buffer and be ready for processing.
/// @details Schedule outstanding requests from outstanding buffers to the internal buffer,
/// from which requests are ready to be submitted to the device for processing.
/// @param[in] state A volatile integer variable used to activate (val>0) or 
///		     disactive (val=0) the scheduler.
void* schedule_requests(void* context_state) {
    if (NULL == context_state) {
        printf("Failed at buffer_manager: argument is NULL.\n");
        pthread_exit(NULL);
    }

    HE_QAT_STATUS* active = (HE_QAT_STATUS*)context_state;

    HE_QAT_TaskRequestList outstanding_requests;
    for (unsigned int i = 0; i < HE_QAT_BUFFER_SIZE; i++) {
        outstanding_requests.request[i] = NULL;
    }
    outstanding_requests.count = 0;

    // this thread should receive signal from context to exit
    *active = HE_QAT_STATUS_RUNNING;
    while (HE_QAT_STATUS_INACTIVE != *active) {
        // Collect a set of requests from the outstanding buffer
        pull_outstanding_requests(&outstanding_requests, &outstanding,
                                  HE_QAT_BUFFER_SIZE);
        // Submit them to the HE QAT buffer for offloading
        submit_request_list(&he_qat_buffer, &outstanding_requests);
    }

    pthread_exit(NULL);
}

/// @brief Poll responses from a specific QAT instance.
/// @param[in] _inst_config Instance configuration containing the parameter 
/// 			    values to start and poll responses from the accelerator.
static void* start_inst_polling(void* _inst_config) {
    if (NULL == _inst_config) {
        printf(
            "Failed at start_inst_polling: argument is NULL.\n");  //,__FUNC__);
        pthread_exit(NULL);
    }

    HE_QAT_InstConfig* config = (HE_QAT_InstConfig*)_inst_config;

    if (NULL == config->inst_handle) return NULL;

#ifdef HE_QAT_DEBUG
    printf("Instance ID %d Polling\n",config->inst_id);
#endif

    // What is harmful for polling without performing any operation?
    config->polling = 1;
    while (config->polling) {
        icp_sal_CyPollInstance(config->inst_handle, 0);
        OS_SLEEP(50);
    }

    pthread_exit(NULL);
}

/// @brief
///	Initialize and start multiple instances, their polling thread, 
///	and a single processing thread.
/// 
/// @details
/// 	It initializes multiple QAT instances and launches their respective independent 
///	polling threads that will listen to responses to requests sent to the accelerators
///	concurrently. Then, it becomes the thread that collect the incoming requests stored 
///	in a shared buffer and send them to the accelerator for processing. This is the only 
///	processing thread for requests handled by multiple instances -- unlike when using 
///     multiple instances with the `start_perform_op` function, in which case each instance 
///	has a separate processing thread. The implementation of the multiple instance support 
///	using `start_perform_op` is obsolete and slower. The way is using this function, which 
///	delivers better performance. The scheduling of request offloads uses a 
///	round-robin approach. It collects multiple requests from the internal buffer and then 
///	send them to the multiple accelerator instances to process in a round-robin fashion. 
///	It was designed to support processing requests of different operation types but 
///	currently only supports Modular Exponentiation. 
///
/// @param[in] _config Data structure containing the configuration of multiple instances.
void* start_instances(void* _config) {
    static unsigned int instance_count = 0;
    static unsigned int next_instance = 0;
    
    if (NULL == _config) {
        printf("Failed in start_instances: _config is NULL.\n");
        pthread_exit(NULL);
    }

    HE_QAT_Config* config = (HE_QAT_Config*)_config;
    instance_count = config->count;

    printf("Instance Count: %d\n",instance_count);
    pthread_t* polling_thread = (pthread_t *) malloc(sizeof(pthread_t)*instance_count);
    if (NULL == polling_thread) {
        printf("Failed in start_instances: polling_thread is NULL.\n");
        pthread_exit(NULL);
    }
    unsigned* request_count_per_instance = (unsigned *) malloc(sizeof(unsigned)*instance_count);
    if (NULL == request_count_per_instance) {
        printf("Failed in start_instances: polling_thread is NULL.\n");
        pthread_exit(NULL);
    }
    for (unsigned i = 0; i < instance_count; i++) {
        request_count_per_instance[i] = 0;
    }

    CpaStatus status = CPA_STATUS_FAIL;

    for (unsigned int j = 0; j < config->count; j++) {	    
        // Start from zero or restart after stop_perform_op
        pthread_mutex_lock(&config->inst_config[j].mutex);
        while (config->inst_config[j].active) 
    	    pthread_cond_wait(&config->inst_config[j].ready, 
    			    	&config->inst_config[j].mutex);
    
        // assert(0 == config->active);
        // assert(NULL == config->inst_handle);
    
        status = cpaCyStartInstance(config->inst_config[j].inst_handle);
        config->inst_config[j].status = status;
        if (CPA_STATUS_SUCCESS == status) {
            printf("Cpa CyInstance has successfully started.\n");
            status =
                cpaCySetAddressTranslation(config->inst_config[j].inst_handle, 
				sampleVirtToPhys);
        }
    
        pthread_cond_signal(&config->inst_config[j].ready);
        pthread_mutex_unlock(&config->inst_config[j].mutex);
    
        if (CPA_STATUS_SUCCESS != status) pthread_exit(NULL);

        printf("Instance ID: %d\n",config->inst_config[j].inst_id);

         // Start QAT instance and start polling
         //pthread_t polling_thread;
         if (pthread_create(&polling_thread[j], config->inst_config[j].attr, start_inst_polling,
                            (void*)&(config->inst_config[j])) != 0) {
             printf("Failed at creating and starting polling thread.\n");
             pthread_exit(NULL);
         }

         if (pthread_detach(polling_thread[j]) != 0) {
             printf("Failed at detaching polling thread.\n");
             pthread_exit(NULL);
         }
	 
	 config->inst_config[j].active = 1;
	 config->inst_config[j].running = 1;
    
    } // for loop
    
    HE_QAT_TaskRequestList outstanding_requests;
    for (unsigned int i = 0; i < HE_QAT_BUFFER_SIZE; i++) {
        outstanding_requests.request[i] = NULL;
    }
    outstanding_requests.count = 0;

    config->running = 1;
    config->active = 1;
    while (config->running) {
#ifdef HE_QAT_DEBUG
        printf("Try reading request from buffer. Inst #%d\n", next_instance);
#endif
	unsigned long pending = request_count - response_count;
	unsigned long available = max_pending - ((pending < max_pending)?pending:max_pending);
#ifdef HE_QAT_DEBUG
	printf("[CHECK] request_count: %lu response_count: %lu pending: %lu available: %lu\n",
			request_count,response_count,pending,available);
#endif
	while (available < restart_threshold) {
#ifdef HE_QAT_DEBUG
	   printf("[WAIT]\n");
#endif
	   // argument passed in microseconds 
	   OS_SLEEP(RESTART_LATENCY_MICROSEC);
           pending = request_count - response_count;
	   available = max_pending - ((pending < max_pending)?pending:max_pending);
#ifdef HE_QAT_DEBUG
           printf("[CHECK] request_count: %lu response_count: %lu pending: %lu available: %lu\n",
          			request_count,response_count,pending,available);
#endif
	}
#ifdef HE_QAT_DEBUG
	printf("[SUBMIT] request_count: %lu response_count: %lu pending: %lu available: %lu\n",
			request_count,response_count,pending,available);
#endif
	unsigned int max_requests = available;

	// Try consume maximum amount of data from butter to perform requested operation
        read_request_list(&outstanding_requests, &he_qat_buffer, max_requests);

#ifdef HE_QAT_DEBUG
        printf("Offloading %u requests to the accelerator.\n", outstanding_requests.count);
#endif
        for (unsigned int i = 0; i < outstanding_requests.count; i++) {
	   HE_QAT_TaskRequest* request = outstanding_requests.request[i];
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
                printf("Offload request using instance #%d\n", next_instance);
#endif
#ifdef HE_QAT_PERF
                gettimeofday(&request->start, NULL);
#endif
                status = cpaCyLnModExp(
                    config->inst_config[next_instance].inst_handle,
                    (CpaCyGenFlatBufCbFunc)
                        request->callback_func,  // lnModExpCallback,
                    (void*)request, (CpaCyLnModExpOpData*)request->op_data,
                    &request->op_result);
                retry++;
                break;
            case HE_QAT_OP_NONE:
            default:
#ifdef HE_QAT_DEBUG
                printf("HE_QAT_OP_NONE to instance #%d\n", next_instance);
#endif
                retry = HE_QAT_MAX_RETRY;
                break;
            }

	    if (CPA_STATUS_RETRY == status) {
	        printf("CPA requested RETRY\n");
	        printf("RETRY count = %u\n",retry);
		pthread_exit(NULL); // halt the whole system
	    }

        } while (CPA_STATUS_RETRY == status && retry < HE_QAT_MAX_RETRY);

        // Ensure every call to perform operation is blocking for each endpoint
        if (CPA_STATUS_SUCCESS == status) {
	    // Global tracking of number of requests 
	    request_count += 1;
	    request_count_per_instance[next_instance] += 1;
            next_instance = (next_instance + 1) % instance_count;

	    // Wake up any blocked call to stop_perform_op, signaling that now it is
            // safe to terminate running instances. Check if this detereorate
            // performance.
            pthread_cond_signal(&config->inst_config[next_instance].ready);  // Prone to the lost wake-up problem

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
	    printf("Request Submission FAILED\n");
        }

#ifdef HE_QAT_DEBUG
        printf("Offloading completed by instance #%d\n", next_instance-1);
#endif

	// Reset pointer
	outstanding_requests.request[i] = NULL;
	request = NULL;
	
        }// for loop over batch of requests
        outstanding_requests.count = 0;
    }
    pthread_exit(NULL);
}

/// @brief
/// 	Start independent processing and polling threads for an instance.
/// 
/// @details
/// 	It initializes a QAT instance and launches its polling thread to listen 
///     to responses (request outputs) from the accelerator. It is also reponsible 
///	to collect requests from the internal buffer and send them to the accelerator 
///	periodiacally. It was designed to extend to receiving and offloading 
///	requests of different operation types but currently only supports Modular 
///	Exponentiation. 
///
/// @param[in] _inst_config Data structure containing the configuration of a single 
///			    instance.
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
    
    HE_QAT_TaskRequestList outstanding_requests;
    for (unsigned int i = 0; i < HE_QAT_BUFFER_SIZE; i++) {
        outstanding_requests.request[i] = NULL;
    }
    outstanding_requests.count = 0;

    config->running = 1;
    config->active = 1;
    while (config->running) {
#ifdef HE_QAT_DEBUG
        printf("Try reading request from buffer. Inst #%d\n", config->inst_id);
#endif
	unsigned long pending = request_count - response_count;
	unsigned long available = max_pending - ((pending < max_pending)?pending:max_pending);
#ifdef HE_QAT_DEBUG
	printf("[CHECK] request_count: %lu response_count: %lu pending: %lu available: %lu\n",
			request_count,response_count,pending,available);
#endif
	while (available < restart_threshold) {
#ifdef HE_QAT_DEBUG
	   printf("[WAIT]\n");
#endif
	   // argument passed in microseconds 
	   OS_SLEEP(650);
           pending = request_count - response_count;
	   available = max_pending - ((pending < max_pending)?pending:max_pending);
	}
#ifdef HE_QAT_DEBUG
	printf("[SUBMIT] request_count: %lu response_count: %lu pending: %lu available: %lu\n",
			request_count,response_count,pending,available);
#endif
	unsigned int max_requests = available;
	// Try consume maximum amount of data from butter to perform requested operation
        read_request_list(&outstanding_requests, &he_qat_buffer, max_requests);

//	// Try consume data from butter to perform requested operation
//        HE_QAT_TaskRequest* request =
//            (HE_QAT_TaskRequest*)read_request(&he_qat_buffer);
//
//        if (NULL == request) {
//            pthread_cond_signal(&config->ready);
//            continue;
//        }
#ifdef HE_QAT_DEBUG
        printf("Offloading %u requests to the accelerator.\n", outstanding_requests.count);
#endif
        for (unsigned int i = 0; i < outstanding_requests.count; i++) {
	   HE_QAT_TaskRequest* request = outstanding_requests.request[i];
#ifdef HE_QAT_SYNC_MODE
        COMPLETION_INIT(&request->callback);
#endif
        unsigned retry = 0;
        do {
            // Realize the type of operation from data
            switch (request->op_type) {
            // Select appropriate action
            case HE_QAT_OP_MODEXP:
		//if (retry > 0) printf("Try offloading again last request\n");
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

	    if (CPA_STATUS_RETRY == status) {
	        printf("CPA requested RETRY\n");
	        printf("RETRY count: %u\n",retry);
                OS_SLEEP(600);
	    }

        } while (CPA_STATUS_RETRY == status && retry < HE_QAT_MAX_RETRY);

        // Ensure every call to perform operation is blocking for each endpoint
        if (CPA_STATUS_SUCCESS == status) {
	    // Global tracking of number of requests 
	    request_count += 1;
	    //printf("retry_count = %d\n",retry_count);
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

	// Reset pointer
	outstanding_requests.request[i] = NULL;
	request = NULL;

        }// for loop over batch of requests
        outstanding_requests.count = 0;
       	
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
/// 	Stop specified number of instances from running.
/// 
/// @details
/// 	Stop first 'num_inst' number of cpaCyInstance(s), including their polling
/// 	and running threads. Stop runnning and polling instances. 
///     Release QAT instances handles.
///
/// @param[in] config List of all created QAT instances and their configurations.
/// @param[in] num_inst Unsigned integer number indicating first number of
/// 			instances to be terminated.
void stop_perform_op(HE_QAT_InstConfig* config, unsigned num_inst) {
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

/// @brief Stop all running instances. 
/// @details
/// 	Stop all running instances after calling `start_instances()`.
///	It will set the states of the instances to terminate gracefully. 
/// @param[in] _config All QAT instances configurations holding their states.
void stop_instances(HE_QAT_Config* _config) {
    if (NULL == _config) return;
    if (_config->active) _config->active = 0;
    if (_config->running) _config->running = 0;
    stop_perform_op(_config->inst_config, _config->count);
    return ;
}

