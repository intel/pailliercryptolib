
#pragma once

#ifndef _HE_QAT_TYPES_H_
#define _HE_QAT_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cpa.h"
#include "cpa_cy_im.h"

#include <pthread.h>

#define HE_QAT_BUFFER_SIZE 1024

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
    int count;                       // occupied track number of buffer enties
    int next_free_slot;  // nextin index of the next free slot to accommodate a
                         // request
    int next_data_slot;  // nextout index of next request to be processed
    pthread_mutex_t mutex;         //
    pthread_cond_t any_more_data;  // more
    pthread_cond_t any_free_slot;  // less
} HE_QAT_RequestBuffer;

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

#ifdef __cplusplus
}  // close the extern "C" {
#endif

#endif  // _HE_QAT_TYPES_H_
