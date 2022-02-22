
#include "heqat_types.h"
#include "icp_sal_poll.h"
#include "cpa_sample_utils.h"

//CpaStatus get_qat_instances(CpaInstanceHandle *_instances, int *_num_instances=-1)
//{
//   
//}

const int MAX_NUM_INST 8

// Type definitions
enum HEQAT_EXEC_MODE { HEQAT_BLOCK_SYNC=1  };
enum HEQAT_STATUS { HEQAT_SUCCESS = 0, HEQAT_FAIL = 1 };
typedef pthread_t QATInst;
typedef struct QATInstConfigStruct {
    CpaInstanceHandle inst_handle;
    //void *func();
    volatile int polling; 
} QATInstConfig;

// Global variable declarations
QATInst       qat_instances   [MAX_NUM_INST];
QATInstConfig qat_inst_config [MAX_NUM_INST];
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

static void inst_thread()
{
    
}

/// @brief 
/// @function start_inst_polling
/// @param[in] QATInstConfig Holds the parameter values to set up the QAT instance thread.
///                          
static void start_inst_polling(QATInstConfig *config)
{
    if (!config) return ;
    
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
    for (int i = 0; i < MAX_NUM_INSTS; i++) {
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
HEQAT_STATUS acquire_qat_devices()
{
    CpaInstanceHandle _inst_handle = NULL;

    // TODO: @fdiasmor Create a CyGetInstance that retrieves more than one.
    sampleCyGetInstance(&_inst_handle);

    if (_inst_handle == NULL) {
        return HEQAT_FAIL;
    } 

    // Dispatch worker threads
    pthread_attr_t attr;
    cpu_set_t cpus;
    pthread_attr_init(&attr);
    for (int i = 0; i < HEQAT_BLOCK_SYNC; i++) {
        CPU_ZERO(&cpus);
	CPU_SET(i, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);

        // configure thread
	QATInstConfig *config = (QATInstConfig *) malloc(sizeof(QATInstConfig));
        if (config == NULL) return HEQAT_FAIL;
        config->inst_handle = _inst_handle[i];
	pthread_create(&qat_instances[i], &attr, start_inst_polling, config); 
    }


    for (int i = 0; i < HEQAT_BLOCK_SYNC; i++) {
        pthread_
    }

    return ;
}

/// @brief
/// @function release_qat_devices
/// Release QAT instances and tear down QAT execution environment.
void release_qat_devices()
{
    CpaStatus status = CPA_STATUS_FAIL;

    // signal all qat instance

    for (int i = 0; i < MAX_NUM_INST; i++) {
        status = cpaCyStopInstance(qat_inst_config[i].inst_handle);
        if (CPA_STATUS_SUCCESS != status) {
            printf("Failed to stop QAT instance #%d\n",i);
	}
    }
    return ;
}

