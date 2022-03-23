
#define _GNU_SOURCE

#include "he_qat_types.h"
#include "he_qat_context.h"

#include <pthread.h>

//#include <sched.h>

//#include <stdio.h>
//#include <stdlib.h>
//#include <assert.h>
#include <stdint.h>
#include <unistd.h>

#include "cpa_sample_utils.h"
#include "icp_sal_user.h"
#include "icp_sal_poll.h"

#ifdef USER_SPACE
#define MAX_INSTANCES 1024
#else
#define MAX_INSTANCES 1
#endif

#define NUM_ACTIVE_INSTANCES 16

// Global variable declarations
//HE_QAT_Inst he_qat_instances[HE_QAT_MAX_NUM_INST];
//pthread_attr_t he_qat_inst_attr[HE_QAT_MAX_NUM_INST];
//HE_QAT_InstConfig he_qat_inst_config[HE_QAT_MAX_NUM_INST];
// HE_QAT_RequestBuffer he_qat_buffer;
HE_QAT_Inst he_qat_instances[NUM_ACTIVE_INSTANCES];
pthread_attr_t he_qat_inst_attr[NUM_ACTIVE_INSTANCES];
HE_QAT_InstConfig he_qat_inst_config[NUM_ACTIVE_INSTANCES];

extern HE_QAT_RequestBuffer he_qat_buffer;
extern void* start_perform_op(void* _inst_config);
extern void stop_perform_op(void* _inst_config, unsigned num_inst);

CpaInstanceHandle handle = NULL;

static CpaInstanceHandle get_qat_instance()
{
    static CpaInstanceHandle cyInstHandles[MAX_INSTANCES];
    static Cpa16U numInstances = 0;
    static Cpa16U nextInstance = 0;
    CpaStatus status = CPA_STATUS_SUCCESS;

    if (0 == numInstances) {
        //*pCyInstHandle = NULL;
        status = cpaCyGetNumInstances(&numInstances);
        if (numInstances >= MAX_INSTANCES) {
            numInstances = MAX_INSTANCES;
        }
	if (numInstances >= NUM_ACTIVE_INSTANCES) {
	    numInstances = NUM_ACTIVE_INSTANCES;
	}

        printf("Found %d CyInstances.\n",numInstances);
        printf("Next Instance: %d.\n",nextInstance);

        if ((status == CPA_STATUS_SUCCESS) && (numInstances > 0)) {
            status = cpaCyGetInstances(numInstances, cyInstHandles);
            if (status == CPA_STATUS_SUCCESS)
		return cyInstHandles[nextInstance];
                //*pCyInstHandle = cyInstHandles[0];
        }
    
        if (0 == numInstances) {
            PRINT_ERR("No instances found for 'SSL'\n");
            PRINT_ERR("Please check your section names");
            PRINT_ERR(" in the config file.\n");
            PRINT_ERR("Also make sure to use config file version 2.\n");
        }

	return NULL;
    } 

    nextInstance = ((nextInstance + 1) % numInstances);
    printf("Next Instance: %d.\n",nextInstance);
    return cyInstHandles[nextInstance];

}

/// @brief
/// @function acquire_qat_devices
/// Acquire QAT instances and set up QAT execution environment.
HE_QAT_STATUS acquire_qat_devices() {
    CpaStatus status = CPA_STATUS_FAIL;

    // Initialize QAT memory pool allocator
    status = qaeMemInit();
    if (CPA_STATUS_SUCCESS != status) {
        printf("Failed to initialized memory driver.\n");
        return HE_QAT_STATUS_FAIL;  // HEQAT_STATUS_ERROR
    }
#ifdef _DESTINY_DEBUG_VERBOSE
    printf("QAT memory successfully initialized.\n");
#endif

    // Not sure if for multiple instances the id will need to be specified, e.g.
    // "SSL1"
    status = icp_sal_userStartMultiProcess("SSL", CPA_FALSE);
    if (CPA_STATUS_SUCCESS != status) {
        printf("Failed to start SAL user process SSL\n");
        qaeMemDestroy();
        return HE_QAT_STATUS_FAIL;  // HEQAT_STATUS_ERROR
    }
#ifdef _DESTINY_DEBUG_VERBOSE
    printf("SAL user process successfully started.\n");
#endif

    // Potential out-of-scope hazard for segmentation fault
    CpaInstanceHandle _inst_handle[NUM_ACTIVE_INSTANCES];// = NULL;
    // TODO: @fdiasmor Create a CyGetInstance that retrieves more than one.
    //sampleCyGetInstance(&_inst_handle);
    for (unsigned int i = 0; i < NUM_ACTIVE_INSTANCES; i++) {
      _inst_handle[i] = get_qat_instance();
      if (_inst_handle[i] == NULL) {
          printf("Failed to find QAT endpoints.\n");
          return HE_QAT_STATUS_FAIL;
      }
    }

    // sampleCyGetInstance(&handle);
    // if (handle == NULL) {
    //    printf("Failed to find QAT endpoints.\n");
    //    return HE_QAT_STATUS_FAIL;
    //}
#ifdef _DESTINY_DEBUG_VERBOSE
    printf("Found QAT endpoints.\n");
#endif

    // Initialize QAT buffer synchronization attributes
    he_qat_buffer.count = 0;
    he_qat_buffer.next_free_slot = 0;
    he_qat_buffer.next_data_slot = 0;

    // Initialize QAT memory buffer
    for (int i = 0; i < HE_QAT_BUFFER_SIZE; i++) {
        he_qat_buffer.data[i] = NULL;
    }

    // Creating QAT instances (consumer threads) to process op requests
    pthread_attr_t attr;
    cpu_set_t cpus;
    //for (int i = 0; i < HE_QAT_SYNC; i++) {
    for (int i = 0; i < NUM_ACTIVE_INSTANCES; i++) {
        CPU_ZERO(&cpus);
        CPU_SET(i, &cpus);
        pthread_attr_init(&he_qat_inst_attr[i]);
        pthread_attr_setaffinity_np(&he_qat_inst_attr[i], sizeof(cpu_set_t),
                                    &cpus);

        // configure thread
        // HE_QAT_InstConfig *config = (HE_QAT_InstConfig *)
        //                                     malloc(sizeof(QATInstConfig));
        // if (config == NULL) return HE_QAT_FAIL;
        he_qat_inst_config[i].active = 0;   // HE_QAT_STATUS_INACTIVE
        he_qat_inst_config[i].polling = 0;  // HE_QAT_STATUS_INACTIVE
        he_qat_inst_config[i].running = 0;
        he_qat_inst_config[i].status = CPA_STATUS_FAIL;
        //	he_qat_inst_config[i].mutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_init(&he_qat_inst_config[i].mutex, NULL);
        //	he_qat_inst_config[i].ready = PTHREAD_COND_INITIALIZER;
        pthread_cond_init(&he_qat_inst_config[i].ready, NULL);
        he_qat_inst_config[i].inst_handle = _inst_handle[i];
        he_qat_inst_config[i].inst_id = i;
        he_qat_inst_config[i].attr = &he_qat_inst_attr[i];
        pthread_create(&he_qat_instances[i], he_qat_inst_config[i].attr,
                       start_perform_op, (void*)&he_qat_inst_config[i]);
    }
#ifdef _DESTINY_DEBUG_VERBOSE
    printf("Created processing threads.\n");
#endif

    // Dispatch the qat instances to run independently in the background
    //for (int i = 0; i < HE_QAT_SYNC; i++) {
    for (int i = 0; i < NUM_ACTIVE_INSTANCES; i++) {
        pthread_detach(he_qat_instances[i]);
    }
#ifdef _DESTINY_DEBUG_VERBOSE
    printf("Detached processing threads.\n");
#endif

    return HE_QAT_STATUS_SUCCESS;
}

/// @brief
/// @function release_qat_devices
/// Release QAT instances and tear down QAT execution environment.
HE_QAT_STATUS release_qat_devices() {
    CpaStatus status = CPA_STATUS_FAIL;

    // signal all qat instance to stop polling
    //    stop_inst_polling();
    //
    //    // Release QAT instances handles
    //    for (int i = 0; i < HE_QAT_MAX_NUM_INST; i++) {
    //        status = cpaCyStopInstance(he_qat_inst_config[i].inst_handle);
    //        if (CPA_STATUS_SUCCESS != status) {
    //            printf("Failed to stop QAT instance #%d\n",i);
    //	    return HE_QAT_STATUS_FAIL;
    //	}
    //    }

    stop_perform_op(he_qat_inst_config, NUM_ACTIVE_INSTANCES);
#ifdef _DESTINY_DEBUG_VERBOSE
    printf("Stopped polling and processing threads.\n");
#endif

    // Deallocate memory of QAT InstConfig

    // Stop QAT SSL service
    icp_sal_userStop();
#ifdef _DESTINY_DEBUG_VERBOSE
    printf("Stopped SAL user process.\n");
#endif

    // Release QAT allocated memory
    qaeMemDestroy();
#ifdef _DESTINY_DEBUG_VERBOSE
    printf("Release QAT memory.\n");
#endif

    return HE_QAT_STATUS_SUCCESS;
}
