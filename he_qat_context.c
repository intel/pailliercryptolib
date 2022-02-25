
#include "he_qat_types.h"
#include "he_qat_context.h"

#include <pthread.h>
#include "icp_sal_poll.h"
#include "cpa_sample_utils.h"

// Global variable declarations
HE_QAT_Inst          he_qat_instances   [HE_QAT_MAX_NUM_INST];
HE_QAT_InstConfig    he_qat_inst_config [HE_QAT_MAX_NUM_INST];
HE_QAT_RequestBuffer he_qat_buffer;

extern void *perform_op(void *_inst_config);

//CpaStatus get_qat_instances(CpaInstanceHandle *_instances, int *_num_instances=-1)
//{
//   
//}

/*
#define HEQAT_FREE_QATInstConfig(config)     \
        do {                                 \
            if (config) {                    \
                if (config->inst_handle) {   \
                
		}                            \
		free(config);                \
		config = NULL;               \
	    }                                \
        } while (0)
*/

/// @brief 
/// @function start_inst_polling
/// @param[in] QATInstConfig Holds the parameter values to set up the QAT instance thread.
///                          
static void start_inst_polling(HE_QAT_InstConfig *config)
{
    if (!config) return ;
    if (!config->inst_handle) return ;

    CpaStatus status = CPA_STATUS_FAIL;
    status = cpaCyStartInstance(config->inst_handle);
    if (CPA_STATUS_SUCCESS == status) {
        status = cpaCySetAddressTranslation(config->inst_handle,
                          sampleVirtToPhys);
    }

    // What is harmful for polling without performing any operation?
    config->polling = 1;
    while (config->polling) {
        icp_sal_CyPollInstance(config->inst_handle, 0);
	OS_SLEEP(10);
    }

    return ;
}

/// @brief This function  
/// @function sal_stop_inst_polling
/// QATInstConfig Holds the parameter values to set up the QAT instance thread.
///                          
static void stop_inst_polling()
{
    for (int i = 0; i < HE_QAT_MAX_NUM_INSTS; i++) {
	// Or check if the cpa_instance is not NULL (try it out)
	//if (!qat_inst_config[i].inst_handle) continue;
	if (!qat_inst_config[i].polling) continue;	
	qat_inst_config[i].polling = 0;
	OS_SLEEP(10);
    }

    return ;
}

/// @brief 
/// @function acquire_qat_devices
/// Acquire QAT instances and set up QAT execution environment.
HE_QAT_STATUS acquire_qat_devices()
{
    CpaStatus status = CPA_STATUS_FAIL;

    // Initialize QAT memory pool allocator
    status = qaeMemInit();
    if (CPA_STATUS_SUCCESS != status) {
        return HE_QAT_STATUS_FAIL; // HEQAT_STATUS_ERROR
    }

    // Not sure if for multiple instances the id will need to be specified, e.g. "SSL1"
    status = icp_sal_userStartMultiProcess("SSL", CPA_FALSE);
    if (CPA_STATUS_SUCCESS != status) {
        printf("Failed to start user process SSL\n");
        qaeMemDestroy();
        return HE_QAT_STATUS_FAIL; // HEQAT_STATUS_ERROR
    }

    CpaInstanceHandle _inst_handle = NULL;
    // TODO: @fdiasmor Create a CyGetInstance that retrieves more than one.
    sampleCyGetInstance(&_inst_handle);
    if (_inst_handle == NULL) {
        return HE_QAT_STATUS_FAIL;
    } 
    
    // Initialize QAT buffer synchronization attributes
    he_qat_buffer.count = 0;
    he_qat_buffer.next_free_slot = 0;
    he_qat_buffer.next_data_slot = 0;

    // Initialize QAT memory buffer
    for (int i = 0; i < HE_QAT_BUFFER_SIZE; i++) {
        // preallocate for 1024, 2048, 4096 of worksize/or less 
	// 
        he_qat_buffer.data[i] = NULL;
    }

    // Creating QAT instances (consumer threads) to process op requests
    pthread_attr_t attr;
    cpu_set_t cpus;
    pthread_attr_init(&attr);
    for (int i = 0; i < HE_QAT_SYNC; i++) {
        CPU_ZERO(&cpus);
	CPU_SET(i, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);

        // configure thread
	HE_QAT_InstConfig *config = (HE_QAT_InstConfig *) 
                                             malloc(sizeof(QATInstConfig));
        if (config == NULL) return HE_QAT_FAIL;
        config->inst_handle = _inst_handle[i];
	pthread_create(&he_qat_instances[i], &attr, start_inst_polling, config); 
    }

    // Dispatch the qat instances to run independently in the background
    for (int i = 0; i < HE_QAT_SYNC; i++) {
        pthread_detach(qat_instances[i]);
    }


    return ;
}

/// @brief
/// @function release_qat_devices
/// Release QAT instances and tear down QAT execution environment.
HE_QAT_STATUS release_qat_devices()
{
    CpaStatus status = CPA_STATUS_FAIL;

    // signal all qat instance to stop polling
    stop_inst_polling();

    // Release QAT instances handles
    for (int i = 0; i < HE_QAT_MAX_NUM_INST; i++) {
        status = cpaCyStopInstance(he_qat_inst_config[i].inst_handle);
        if (CPA_STATUS_SUCCESS != status) {
            printf("Failed to stop QAT instance #%d\n",i);
	    return HE_QAT_STATUS_FAIL;
	}
    }

    // Deallocate memory of QAT InstConfig

    // Stop QAT SSL service
    icp_sal_userStop();

    // Release QAT allocated memory 
    qaeMemDestroy();

    return HE_QAT_STATUS_SUCCESS;
}

