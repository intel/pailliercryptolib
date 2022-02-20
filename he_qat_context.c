
#include "heqat_types.h"

#include "cpa_sample_utils.h"

//CpaStatus get_qat_instances(CpaInstanceHandle *_instances, int *_num_instances=-1)
//{
//   
//}

const int MAX_NUM_INST 8

enum HEQAT_EXEC_MODE { HEQAT_BLOCK_SYNC=1  };
enum HEQAT_STATUS { HEQAT_SUCCESS = 0, HEQAT_FAIL = 1 };


typedef struct InstConfigStruct {
    CpaInstanceHandle inst_handle;
    volatile int polling; 
} InstConfig;

pthread_t threads[MAX_NUM_INST];

static void sal_inst_polling(InstConfig *config)
{
    if (!config) return ;
    
    config->polling = 1;
    
    while (config->polling) {
        icp_sal_CyPollInstance(config->inst_handle, 0);
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
	InstConfig *config = (InstConfig *) malloc(sizeof(InstConfig));
        if (config == NULL) return HEQAT_FAIL;
        config->inst_handle = _inst_handle[i];
	pthread_create(&threads[i], &attr, sal_polling, config); //
    }


    return ;
}

/// @brief
/// @function release_qat_devices
/// Release QAT instances and tear down QAT execution environment.
void release_qat_devices()
{
    return ;
}

