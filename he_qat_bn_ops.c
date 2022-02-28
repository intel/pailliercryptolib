
#include "cpa.h"
#include "cpa_cy_im.h"
#include "cpa_cy_ln.h"
#include "icp_sal_poll.h"

#include "cpa_sample_utils.h"

#include "he_qat_types.h"
#include "he_qat_bn_ops.h"

#include <pthread.h>
#include <assert.h>

// Global buffer for the runtime environment
HE_QAT_RequestBuffer he_qat_buffer;
//he_qat_buffer.count = 0;

/// @brief 
/// @function 
/// Callback function for lnModExpPerformOp. It performs any data processing 
/// required after the modular exponentiation.
static void lnModExpCallback(void *pCallbackTag, // This type can be variable
                          CpaStatus status,
                          void *pOpData,         // This is fixed -- please swap it
                          CpaFlatBuffer *pOut)
{
    HE_QAT_TaskRequest *request = NULL;

    //if (CPA_STATUS_SUCCESS != status) {
    //    // Update request status as an error (in pOpData)
    //    
    //    return ;
    //}

    // Check if input data for the op is available and do something
    if (NULL != pCallbackTag) {
        // Read request data
        request = (HE_QAT_TaskRequest *) pCallbackTag;

        // Collect the device output in pOut
		
        request->op_status = status;
	if (pOpData == &request->op_data)
	    // Mark request as complete or ready to be used
            request->request_status = HE_QAT_READY;
	else 
            request->request_status = HE_QAT_STATUS_FAIL;
        
	COMPLETE((struct COMPLETION_STRUCT *)&request->callback);
    }

    // Asynchronous call needs to send wake-up signal to semaphore  
   // if (NULL != pCallbackTag) {
   //     COMPLETE((struct COMPLETION_STRUCT *)pCallbackTag);
   // }

    return ;
}


/// @brief
/// @function
/// Thread-safe producer implementation for the shared request buffer.
/// Stores requests in a buffer that will be offload to QAT devices.
static void submit_request(HE_QAT_RequestBuffer *_buffer, void *args)
{
    pthread_mutex_lock(&_buffer->mutex);
   
    while (_buffer->count >= HE_QAT_BUFFER_SIZE)
        pthread_cond_wait(&_buffer->any_free_slot, &_buffer->mutex);

    assert(_buffer->count < HE_QAT_BUFFER_SIZE);

    _buffer->data[_buffer->next_free_slot++] = args;

    _buffer->next_free_slot %= HE_QAT_BUFFER_SIZE;
    _buffer->count++;

    /* now: either b->occupied < BSIZE and b->nextin is the index
       of the next empty slot in the buffer, or
       b->occupied == BSIZE and b->nextin is the index of the
       next (occupied) slot that will be emptied by a consumer
       (such as b->nextin == b->nextout) */

    pthread_cond_signal(&_buffer->any_more_data);
    pthread_mutex_unlock(&_buffer->mutex);
}

/// @brief
/// @function
/// Thread-safe consumer implementation for the shared request buffer.
/// Read requests from a buffer to finally offload the work to QAT devices.
static HE_QAT_TaskRequest *read_request(HE_QAT_RequestBuffer *_buffer)
{
    void *item = NULL;
    pthread_mutex_lock(&_buffer->mutex);
    while(_buffer->count <= 0)
        pthread_cond_wait(&_buffer->any_more_data, &_buffer->mutex);

    assert(_buffer->count > 0);

    //printf("[%02d]:",_buffer->next_data_slot);
    item = _buffer->data[_buffer->next_data_slot++];

    // make copy of request so that the buffer can be reused

    _buffer->next_data_slot %= HE_QAT_BUFFER_SIZE;
    _buffer->count--;

    /* now: either b->occupied > 0 and b->nextout is the index
       of the next occupied slot in the buffer, or
       b->occupied == 0 and b->nextout is the index of the next
       (empty) slot that will be filled by a producer (such as
       b->nextout == b->nextin) */

    pthread_cond_signal(&_buffer->any_free_slot);
    pthread_mutex_unlock(&_buffer->mutex);

    return (HE_QAT_TaskRequest *) (item);
}

/// @brief 
/// @function start_inst_polling
/// @param[in] HE_QAT_InstConfig Parameter values to start and poll instances.
///                          
static void *start_inst_polling (void *_inst_config)
{
    if (NULL == _inst_config) {
        printf("Failed at start_inst_polling: argument is NULL."); //,__FUNC__); 
        pthread_exit(NULL);
    }

    HE_QAT_InstConfig *config = (HE_QAT_InstConfig *) _inst_config;

    if (NULL == config->inst_handle) return NULL;

    // What is harmful for polling without performing any operation?
    config->polling = 1;
    while (config->polling) {
        icp_sal_CyPollInstance(config->inst_handle, 0);
	OS_SLEEP(10);
    }

    pthread_exit(NULL);
}

/// @brief This function  
/// @function stop_inst_polling
/// Stop polling instances and halt polling thread.     
static void stop_inst_polling(HE_QAT_InstConfig *config)
{
    config->polling = 0;
    OS_SLEEP(10);
    return ;
}

/// @brief 
/// @function perform_op
/// Offload operation to QAT endpoints; for example, large number modular exponentiation.
/// @param[in] HE_QAT_InstConfig *: contains the handle to CPA instance, pointer the global buffer of requests.
void *start_perform_op(void *_inst_config)
{
    if (NULL == _inst_config) {
        printf("Failed in start_perform_op: _inst_config is NULL.\n");
        pthread_exit(NULL);
    }

    HE_QAT_InstConfig *config = (HE_QAT_InstConfig *) _inst_config;

    CpaStatus status = CPA_STATUS_FAIL;

    // If called again, wait
    pthread_mutex_lock(&config->mutex);
    while (config->active)
        pthread_cond_wait(&config->ready, &config->mutex);

    //assert(0 == config->active);
    //assert(NULL == config->inst_handle);

    status = cpaCyStartInstance(config->inst_handle);
    config->status = status;
    if (CPA_STATUS_SUCCESS == status) {
	printf("Cpa CyInstance has successfully started.\n ");
        status = cpaCySetAddressTranslation(config->inst_handle,
                          sampleVirtToPhys);
        
    } 

    pthread_cond_signal(&config->ready);
    pthread_mutex_unlock(&config->mutex);
    
    if (CPA_STATUS_SUCCESS != status) 
        pthread_exit(NULL);

    // Start QAT instance and start polling
    pthread_t polling_thread;
    if (pthread_create(&polling_thread, config->attr, 
			    start_inst_polling, (void *) config) != 0) {
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
	// Try consume data from butter to perform requested operation
	HE_QAT_TaskRequest *request = NULL; 
	           // (HE_QAT_TaskRequest *) read_request(config->he_qat_buffer);

        if (NULL == request) { 
            pthread_cond_signal(&config->ready);
            continue;
        }

        COMPLETION_INIT(&request->callback);

        unsigned retry = 0;
        do {
	    // Realize the type of operation from data
            switch (request->op_type) { 
	        // Select appropriate action
                case HE_QAT_MODEXP:
                    status = cpaCyLnModExp(config->inst_handle,
        			    lnModExpCallback,
        			    (void *) request,
        			    (CpaCyLnModExpOpData *) request->op_data,
        			    &request->op_result);
		    retry++;
		    break;
		case HE_QAT_NO_OP:
		default:
		    retry = HE_QAT_MAX_RETRY; 
		    break;
	    }
            
        } while (CPA_STATUS_RETRY == status && retry < HE_QAT_MAX_RETRY);

	// Ensure every call to perform operation is blocking for each endpoint
        if (CPA_STATUS_SUCCESS == status) {
            // Wait until the callback function has been called
	    if (0 != COMPLETION_WAIT(&request->callback, TIMEOUT_MS)) {
                request->op_status = CPA_STATUS_FAIL;
		request->request_status = HE_QAT_STATUS_FAIL; // Review it
		//request->request_status = HE_QAT_STATUS_INCOMPLETE;
	    }
	}


        // Wake up any blocked call to stop_perform_op, signaling that now it is 
	// safe to terminate running instances. Check if this detereorate performance.
        pthread_cond_signal(&config->ready);

	// Update the status of the request
	//request->op_status = status;
	//if (CPA_STATUS_SUCCESS != status) 
        //    request->request_status = HE_QAT_FAIL;
	//else 
        //    request->request_status = HE_QAT_READY;
    }
    pthread_exit(NULL);
}

/// @brief 
/// @function
/// Stop first 'num_inst' number of cpaCyInstance(s), including their polling and running threads.
/// @param[in] HE_QAT_InstConfig config Vector of created instances with their configuration setup.
/// @param[in] num_inst Unsigned integer number indicating first number of instances to be terminated.
void stop_perform_op(HE_QAT_InstConfig *config, unsigned num_inst)
{
    //if () {
    // Stop runnning and polling instances
    // Release QAT instances handles
    if (NULL == config) return ;

    CpaStatus status = CPA_STATUS_FAIL;
    for (unsigned i = 0; i < num_inst; i++) {
	    pthread_mutex_lock(&config[i].mutex);
#ifdef _DESTINY_DEBUG_VERBOSE
            printf("Try teardown HE QAT instance #%d.\n",i);
#endif
	    while (0 == config[i].active) {
	        pthread_cond_wait(&config[i].ready, &config[i].mutex);
	    }
	    if (CPA_STATUS_SUCCESS == config[i].status && config[i].active) { 
#ifdef _DESTINY_DEBUG_VERBOSE
	        printf("Stop polling and running threads #%d\n",i);
#endif
		config[i].polling = 0;
	        config[i].running = 0;
	        OS_SLEEP(10);
#ifdef _DESTINY_DEBUG_VERBOSE
	        printf("Stop cpaCyInstance #%d\n",i);
#endif
		if (config[i].inst_handle == NULL) continue;
#ifdef _DESTINY_DEBUG_VERBOSE
	        printf("cpaCyStopInstance\n");
#endif
		status = cpaCyStopInstance(config[i].inst_handle);
                if (CPA_STATUS_SUCCESS != status) {
                    printf("Failed to stop QAT instance #%d\n",i);
	        }
	    }
	    pthread_cond_signal(&config[i].ready);
	    pthread_mutex_unlock(&config[i].mutex);
    }
    //}


    return ;
}

HE_QAT_STATUS bnModExpPerformOp(BIGNUM *r, BIGNUM *b, BIGNUM *e, BIGNUM *m, int nbits)
{
    // Unpack data and copy to QAT friendly memory space
    int len = nbits / 8;

    Cpa8U *pBase = NULL;
    Cpa8U *pModulus = NULL;
    Cpa8U *pExponent = NULL;

    HE_QAT_TaskRequest *request = (HE_QAT_TaskRequest *) calloc(1,sizeof(HE_QAT_TaskRequest));
    if (NULL == request) {
         printf("HE_QAT_TaskRequest memory allocation failed in bnModExpPerformOp.\n");
	 return HE_QAT_STATUS_FAIL;
    }
   
    // TODO: @fdiasmor Try it with 8-byte alignment.
    CpaStatus status = PHYS_CONTIG_ALLOC(&pBase, len);
    if (CPA_STATUS_SUCCESS == status && NULL != pBase) {
        unsigned char *bin = (unsigned char *) calloc(len, sizeof(unsigned char));
	if (BN_bn2binpad(b, bin, len)) { 
	   memcpy(pBase, bin, len);
	} else { 
           printf("BN_bn2binpad failed in bnModExpPerformOp.\n");
	   PHYS_CONTIG_FREE(pBase);
	   free(bin);
	   bin = NULL;
	   return HE_QAT_STATUS_FAIL; 
	}
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
	return HE_QAT_STATUS_FAIL;
    }

    status = PHYS_CONTIG_ALLOC(&pExponent, len);
    if (CPA_STATUS_SUCCESS == status && NULL != pExponent) {
        unsigned char *bin = (unsigned char *) calloc(len, sizeof(unsigned char));
	if (BN_bn2binpad(e, bin, len)) { 
	   memcpy(pExponent, bin, len);
	} else { 
           printf("BN_bn2binpad failed in bnModExpPerformOp.\n");
	   PHYS_CONTIG_FREE(pExponent);
	   free(bin);
	   bin = NULL;
	   return HE_QAT_STATUS_FAIL; 
	}
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
	return HE_QAT_STATUS_FAIL;
    }

    status = PHYS_CONTIG_ALLOC(&pModulus, len);
    if (CPA_STATUS_SUCCESS == status && NULL != pModulus) {
        unsigned char *bin = (unsigned char *) calloc(len, sizeof(unsigned char));
	if (BN_bn2binpad(m, bin, len)) { 
	   memcpy(pModulus, bin, len);
	} else { 
           printf("BN_bn2binpad failed in bnModExpPerformOp.\n");
	   free(bin);
	   bin = NULL;
	   return HE_QAT_STATUS_FAIL; 
	}
    } else {
        printf("Contiguous memory allocation failed for pBase.\n");
	return HE_QAT_STATUS_FAIL;
    }

    // Pack it as a QAT Task Request  
    CpaCyLnModExpOpData *op_data = (CpaCyLnModExpOpData *) calloc(1, sizeof(CpaCyLnModExpOpData));
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
    request->op_data = (void *) op_data; 
    
    status = PHYS_CONTIG_ALLOC(&request->op_result.pData, len);
    if (CPA_STATUS_SUCCESS == status && NULL != request->op_result.pData) {
        request->op_result.dataLenInBytes = len;
    } else {
        printf("CpaFlatBuffer.pData memory allocation failed in bnModExpPerformOp.\n");
	return HE_QAT_STATUS_FAIL; 
    }

    request->op_status = status;
    request->op_output = (void *) r;

    // Submit request using producer function
    submit_request(&he_qat_buffer, (void *) request);

    return HE_QAT_STATUS_SUCCESS; 
}

// Maybe it will be useful to pass the number of requests to retrieve
// Pass post-processing function as argument to bring output to expected type
void getBnModExpRequest()
{
    unsigned int finish = 0; 
    // TODO: @fdiasmor Introduce global variable that record at which upcoming request it currently is
    while (0 == finish) {
       finish = 1;
       for (unsigned int i = 0; i < HE_QAT_BUFFER_SIZE && finish; i++) {
	   HE_QAT_TaskRequest *task = (HE_QAT_TaskRequest *) he_qat_buffer.data[i];
	   // TODO: @fdiasmor Check if not NULL before read.
	   finish = (HE_QAT_READY == task->request_status);
           if (finish) { // ? 1 : 0;
	       // Set output results to original format
	       BIGNUM *r = BN_bin2bn(task->op_result.pData, task->op_result.dataLenInBytes, 
			       (BIGNUM *) task->op_output);
               
	       // Free up QAT temporary memory
	       CpaCyLnModExpOpData *op_data = (CpaCyLnModExpOpData *) task->op_data;
	       if (op_data) {
	           PHYS_CONTIG_FREE(op_data->base.pData);
                   PHYS_CONTIG_FREE(op_data->exponent.pData);
                   PHYS_CONTIG_FREE(op_data->modulus.pData); 
	       }
	       if (task->op_result.pData) {
		   PHYS_CONTIG_FREE(task->op_result.pData);
	       }

	       // Destroy synchronization object
	       COMPLETION_DESTROY(&task->callback);
	   }
       }
    }
    return ;
}

