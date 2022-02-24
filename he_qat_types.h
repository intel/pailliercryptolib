
#pragma once

#include <pthread.h>

#define HE_QAT_BUFFER_SIZE 4

// Type definitions
enum HE_QAT_EXEC_MODE { 
        HE_QAT_SYNC = 1, 
        HE_QAT_ASYNC = 2  
};

enum HE_QAT_STATUS { 
	HE_QAT_READY = 1,
        HE_QAT_SUCCESS = 0, 
        HE_QAT_FAIL = -1 
};

typedef pthread_t HE_QAT_Inst;

typedef struct {
    CpaInstanceHandle inst_handle;
    volatile int polling;
    volatile int running;
} HE_QAT_InstConfig;

typedef struct {
    void *data[HE_QAT_BUFFER_SIZE]; // 
    int count;                       // occupied track number of buffer enties
    int next_free_slot;                  // nextin index of the next free slot to accommodate a request
    int next_data_slot;                  // nextout index of next request to be processed
    pthread_mutex_t mutex;           // 
    pthread_cond_t any_more_data;           // more
    pthread_cond_t any_free_slot;            // less
} HE_QAT_RequestBuffer;

//QATOpRequestBuffer qat_request_buffer;

