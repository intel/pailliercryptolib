
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

        unsigned retry = 0;
        do {
	    // Realize the type of operation from data
            switch (request->op_type) { 
	        // Select appropriate action
                case HE_QAT_MODEXP:
                    status = cpaCyLnModExp(config->inst_handle,
        			    lnModExpCallback,
        			    (void *) request,
        			    &request->op_data,
        			    &request->op_output);
		    retry++;
		    break;
		case HE_QAT_NO_OP:
		default:
		    retry = HE_QAT_MAX_RETRY; 
		    break;
	    }
            
        } while (CPA_STATUS_RETRY == status && retry < HE_QAT_MAX_RETRY);

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

void stop_perform_op(HE_QAT_InstConfig *config, unsigned num_inst)
{
    //if () {
    // Stop runnning and polling instances
    // Release QAT instances handles
    if (NULL == config) return ;

    CpaStatus status = CPA_STATUS_FAIL;
    for (unsigned i = 0; i < num_inst; i++) {
	//if (config[i].polling || config[i].running) { 
	    pthread_mutex_lock(&config[i].mutex);
	    printf("Try stopping instance #%d.\n",i);
	    while (0 == config[i].active) {
		printf("STUCK\n");
	        pthread_cond_wait(&config[i].ready, &config[i].mutex);
	    }
	    if (CPA_STATUS_SUCCESS == config[i].status && config[i].active) { 
	        printf("Stopping polling and running instance #%d\n",i);
	        config[i].polling = 0;
	        config[i].running = 0;
	        OS_SLEEP(10);
	        printf("Stopping instance #%d\n",i);
	        if (config[i].inst_handle == NULL) continue;
	        printf("cpaCyStopInstance\n");
                status = cpaCyStopInstance(config[i].inst_handle);
                if (CPA_STATUS_SUCCESS != status) {
                    printf("Failed to stop QAT instance #%d\n",i);
	            //return HE_QAT_STATUS_FAIL;
	        }
	    }
	    printf("Passed once\n");
	    pthread_cond_signal(&config[i].ready);
	    pthread_mutex_unlock(&config[i].mutex);
	//}
    }
    //}


    return ;
}

HE_QAT_STATUS bnModExpPerformOp(BIGNUM *r, BIGNUM *b, BIGNUM *e, BIGNUM *m, int nbits)
{
   // Unpack data and copy to QAT friendly memory space
   // Pack it as a QAT Task Request
   //

   return HE_QAT_STATUS_SUCCESS; 
}

