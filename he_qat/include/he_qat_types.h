
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
#include "cpa_sample_utils.h"

// C Libraries
#include <pthread.h>
#ifdef HE_QAT_PERF
#include <sys/time.h>
#endif

// Local Constants
#define HE_QAT_NUM_ACTIVE_INSTANCES 8
#define HE_QAT_BUFFER_SIZE 1024
#define HE_QAT_BUFFER_COUNT HE_QAT_NUM_ACTIVE_INSTANCES
#define HE_QAT_MAX_RETRY 100

// Type definitions
typedef enum { HE_QAT_SYNC = 1, HE_QAT_ASYNC = 2 } HE_QAT_EXEC_MODE;

typedef enum {
    HE_QAT_STATUS_INVALID_PARAM = 2,
    HE_QAT_STATUS_READY = 1,
    HE_QAT_STATUS_SUCCESS = 0,
    HE_QAT_STATUS_FAIL = -1
} HE_QAT_STATUS;

typedef enum { HE_QAT_OP_NONE = 0, HE_QAT_OP_MODEXP = 1 } HE_QAT_OP;

typedef pthread_t HE_QAT_Inst;

typedef struct {
    void* data[HE_QAT_BUFFER_SIZE];  //
    volatile unsigned int count;     // occupied track number of buffer enties
    // nextin index of the next free slot for a request
    unsigned int next_free_slot;
    // nextout index of next request to be processed
    unsigned int next_data_slot;
    // index of next output data to be read by a thread waiting
    // for all the request to complete processing
    unsigned int next_data_out;
    pthread_mutex_t mutex;         //
    pthread_cond_t any_more_data;  // more
    pthread_cond_t any_free_slot;  // less
} HE_QAT_RequestBuffer;

typedef struct {
    void* data[HE_QAT_BUFFER_COUNT][HE_QAT_BUFFER_SIZE];  //
    unsigned int count;
    unsigned int size;
    unsigned int
        next_free_slot;  // nextin index of the next free slot for a request
    unsigned int
        next_data_slot;     // nextout index of next request to be processed
    pthread_mutex_t mutex;  //
    pthread_cond_t any_more_data;  // more
    pthread_cond_t any_free_slot;  // less
} HE_QAT_RequestBufferList;

typedef struct {
    HE_QAT_RequestBuffer buffer[HE_QAT_BUFFER_COUNT];  //
    unsigned int busy_count;
    unsigned int
        next_free_buffer;  // nextin index of the next free slot for a request
    int free_buffer[HE_QAT_BUFFER_COUNT];
    unsigned int
        next_ready_buffer;  // nextout index of next request to be processed
    int ready_buffer[HE_QAT_BUFFER_COUNT];
    pthread_mutex_t mutex;            //
    pthread_cond_t any_ready_buffer;  // more
    pthread_cond_t any_free_buffer;   // less
} HE_QAT_OutstandingBuffer;

typedef struct {
    int inst_id;
    CpaInstanceHandle inst_handle;
    pthread_attr_t* attr;
    HE_QAT_RequestBuffer* he_qat_buffer;
    pthread_mutex_t mutex;
    pthread_cond_t ready;
    volatile int active;
    volatile int polling;
    volatile int running;
    CpaStatus status;
} HE_QAT_InstConfig;

typedef struct {
   HE_QAT_InstConfig *inst_config;
   volatile int active;
   volatile int running;
   unsigned int count;
} HE_QAT_Config;

// One for each consumer
typedef struct {
    unsigned long long id; 
    // sem_t callback;
    struct COMPLETION_STRUCT callback;
    HE_QAT_OP op_type;
    CpaStatus op_status;
    CpaFlatBuffer op_result;
    // CpaCyLnModExpOpData op_data;
    void* op_data;
    void* op_output;
    void* callback_func;
    volatile HE_QAT_STATUS request_status;
    pthread_mutex_t mutex;
    pthread_cond_t ready;
#ifdef HE_QAT_PERF
    struct timeval start;
    struct timeval end;
#endif
} HE_QAT_TaskRequest;

// One for each consumer
typedef struct {
    HE_QAT_TaskRequest* request[HE_QAT_BUFFER_SIZE];
    unsigned int count;
} HE_QAT_TaskRequestList;

#ifdef __cplusplus
}  // close the extern "C" {
#endif

#endif  // _HE_QAT_TYPES_H_
