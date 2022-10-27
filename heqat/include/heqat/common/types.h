/// @file heqat/common/types.h

#pragma once

#ifndef _HE_QAT_TYPES_H_
#define _HE_QAT_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

// QATLib Headers
#include "cpa.h"
#include "cpa_cy_im.h"
#include "cpa_cy_ln.h"


// C Libraries
#include <pthread.h>
#ifdef HE_QAT_PERF
#include <sys/time.h>
#endif

#include "heqat/common/cpa_sample_utils.h"
#include "heqat/common/consts.h"

// Type definitions
typedef enum { 
    HE_QAT_SYNC = 1, 
    HE_QAT_ASYNC = 2 
} HE_QAT_EXEC_MODE;

typedef enum {
    HE_QAT_STATUS_ACTIVE = 3,
    HE_QAT_STATUS_RUNNING = 4,
    HE_QAT_STATUS_INVALID_PARAM = 2,
    HE_QAT_STATUS_READY = 1,
    HE_QAT_STATUS_SUCCESS = 0,
    HE_QAT_STATUS_FAIL = -1,
    HE_QAT_STATUS_INACTIVE = -2
} HE_QAT_STATUS;

typedef enum { 
    HE_QAT_OP_NONE = 0,  ///< No Operation (NO OP)
    HE_QAT_OP_MODEXP = 1 ///< QAT Modular Exponentiation
} HE_QAT_OP;

typedef pthread_t HE_QAT_Inst;

typedef struct {
    void* data[HE_QAT_BUFFER_SIZE]; ///< Stores work requests ready to be sent to the accelerator.
    volatile unsigned int count; ///< Tracks the number of occupied entries/slots in the data[] buffer.
    // nextin index of the next free slot for a request
    unsigned int next_free_slot; ///< Index of the next slot available to store a new request.
    // nextout index of next request to be processed
    unsigned int next_data_slot; ///< Index of the next slot containing request ready to be consumed for processing.
    // index of next output data to be read by a thread waiting
    // for all the request to complete processing
    unsigned int next_data_out; ///< Index of the next slot containing request whose processing has been completed and its output is ready to be consumed by the caller.
    pthread_mutex_t mutex; ///< Synchronization object used to control access to the buffer.
    pthread_cond_t any_more_data; ///< Conditional variable used to synchronize the consumption of data in buffer (wait until more data is available to be consumed).
    pthread_cond_t any_free_slot; ///< Conditional variable used to synchronize the provision of free slots in buffer (wait until enough slots are available to add more data in buffer).
} HE_QAT_RequestBuffer;

typedef struct {
    HE_QAT_RequestBuffer buffer[HE_QAT_BUFFER_COUNT];  ///< Buffers to support concurrent threads with less sync overhead. Stores incoming request from different threads.
    unsigned int busy_count; ///< Counts number of currently occupied buffers.
    unsigned int next_free_buffer; ///< Next in: index of the next free slot for a request.
    int free_buffer[HE_QAT_BUFFER_COUNT]; ///< Keeps track of buffers that are available (any value > 0 means the buffer at index i is available).  The next_free_buffer does not necessarily mean that the buffer is already released from usage.
    unsigned int next_ready_buffer;  ///< Next out: index of next request to be processed.
    int ready_buffer[HE_QAT_BUFFER_COUNT]; ///< Keeps track of buffers that are ready (any value > 0 means the buffer at index i is ready). The next_ready_buffer does not necessarily mean that the buffer is not busy at any time instance.
    pthread_mutex_t mutex; ///< Used for synchronization of concurrent access of an object of the type
    pthread_cond_t any_ready_buffer; ///< Conditional variable used to synchronize the consumption of the contents in the buffers storing outstanding requests and ready to be scheduled to move to the internal buffer.
    pthread_cond_t any_free_buffer; ///< Conditional variable used to synchronize the provisioning of buffers to store incoming requests from concurrent threads.
} HE_QAT_OutstandingBuffer;

typedef struct {
    int inst_id; ///< QAT instance ID.
    CpaInstanceHandle inst_handle; ///< Handle of this QAT instance.
    pthread_attr_t* attr; ///< Unused member.
    HE_QAT_RequestBuffer* he_qat_buffer; ///< Unused member.
    pthread_mutex_t mutex;
    pthread_cond_t ready;
    volatile int active; ///< State of this QAT instance.  
    volatile int polling; ///< State of this QAT instance's polling thread (any value different from 0 indicates that it is running). 
    volatile int running; ///< State of this QAT instance's processing thread (any value different from 0 indicates that it is running).
    CpaStatus status; ///< Status of the latest activity by this QAT instance.
} HE_QAT_InstConfig;

typedef struct {
    HE_QAT_InstConfig *inst_config; ///< List of the QAT instance's configurations.
    volatile int active; ///< Value different from 0 indicates all QAT instances are created and active.
    volatile int running; ///< Value different from 0 indicates all created QAT instances are running.
    unsigned int count; ///< Total number of created QAT instances.
} HE_QAT_Config;

// One for each consumer
typedef struct {
    unsigned long long id; ///< Work request ID. 
    // sem_t callback;
    struct COMPLETION_STRUCT callback; ///< Synchronization object.
    HE_QAT_OP op_type; ///< Work type: type of operation to be offloaded to QAT.
    CpaStatus op_status; ///< Status of the operation after completion.
    CpaFlatBuffer op_result; ///< Output of the operation in contiguous memory.
    // CpaCyLnModExpOpData op_data;
    void* op_data; ///< Input data packaged in QAT's data structure for the target type of operation.
    void* op_output; ///< Pointer to the memory space where to store the output for the caller.
    void* callback_func; ///< Pointer to the callback function.
    volatile HE_QAT_STATUS request_status; 
    pthread_mutex_t mutex;
    pthread_cond_t ready;
#ifdef HE_QAT_PERF
    struct timeval start; ///< Time when the request was first received from the caller.
    struct timeval end; ///< Time when the request completed processing and callback function was triggered.
#endif
} HE_QAT_TaskRequest;

typedef struct {
    HE_QAT_TaskRequest* request[HE_QAT_BUFFER_SIZE];
    unsigned int count;
} HE_QAT_TaskRequestList;

#ifdef __cplusplus
}  // close the extern "C" {
#endif

#endif  // _HE_QAT_TYPES_H_
