// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
/// @file heqat/context.c

#define _GNU_SOURCE

#include <icp_sal_user.h>
#include <icp_sal_poll.h>
#include <qae_mem.h>

#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#include "heqat/common/types.h"
#include "heqat/common/utils.h"
#include "heqat/context.h"

#ifdef USER_SPACE
#define MAX_INSTANCES 1024
#else
#define MAX_INSTANCES 1
#endif

// Utilities functions from qae_mem.h header
extern CpaStatus qaeMemInit(void);
extern void qaeMemDestroy(void);

static volatile HE_QAT_STATUS context_state = HE_QAT_STATUS_INACTIVE;
static pthread_mutex_t context_lock;

// Global variable declarations
static pthread_t buffer_manager;
static pthread_t he_qat_runner;
static pthread_attr_t he_qat_inst_attr[HE_QAT_NUM_ACTIVE_INSTANCES];
static HE_QAT_InstConfig he_qat_inst_config[HE_QAT_NUM_ACTIVE_INSTANCES];
static HE_QAT_Config* he_qat_config = NULL;

// External global variables
extern HE_QAT_RequestBuffer he_qat_buffer;
extern HE_QAT_OutstandingBuffer outstanding;

/***********           Internal Services          ***********/
// Start scheduler of work requests (consumer)
extern void* schedule_requests(void* state);
// Activate cpaCyInstances to run on background and poll responses from QAT
// accelerator
extern void* start_instances(void* _inst_config);
// Stop a running group of cpaCyInstances started with the "start_instances"
// service
extern void stop_instances(HE_QAT_Config* _config);
// Stop running individual QAT instances from a list of cpaCyInstances (called
// by "stop_instances")
extern void stop_perform_op(void* _inst_config, unsigned num_inst);
// Activate a cpaCyInstance to run on background and poll responses from QAT
// accelerator WARNING: Deprecated when "start_instances" becomes default.
extern void* start_perform_op(void* _inst_config);

static Cpa16U numInstances = 0;
static Cpa16U nextInstance = 0;

static CpaInstanceHandle get_qat_instance() {
    static CpaInstanceHandle cyInstHandles[MAX_INSTANCES];
    CpaStatus status = CPA_STATUS_SUCCESS;
    CpaInstanceInfo2 info = {0};

    if (0 == numInstances) {
        status = cpaCyGetNumInstances(&numInstances);
        if (numInstances >= MAX_INSTANCES) {
            numInstances = MAX_INSTANCES;
        }
        if (numInstances >= HE_QAT_NUM_ACTIVE_INSTANCES) {
            numInstances = HE_QAT_NUM_ACTIVE_INSTANCES;
        }

        if (CPA_STATUS_SUCCESS != status) {
            HE_QAT_PRINT_ERR("No CyInstances Found (%d).\n", numInstances);
            return NULL;
        }

        HE_QAT_PRINT_DBG("Found %d CyInstances.\n", numInstances);

        if ((status == CPA_STATUS_SUCCESS) && (numInstances > 0)) {
            status = cpaCyGetInstances(numInstances, cyInstHandles);

            // List instances and their characteristics
            for (unsigned int i = 0; i < numInstances; i++) {
                status = cpaCyInstanceGetInfo2(cyInstHandles[i], &info);
                if (CPA_STATUS_SUCCESS != status) return NULL;
#ifdef HE_QAT_DEBUG
                HE_QAT_PRINT("Vendor Name: %s\n", info.vendorName);
                HE_QAT_PRINT("Part Name: %s\n", info.partName);
                HE_QAT_PRINT("Inst Name: %s\n", info.instName);
                HE_QAT_PRINT("Inst ID: %s\n", info.instID);
                HE_QAT_PRINT("Node Affinity: %u\n", info.nodeAffinity);
                HE_QAT_PRINT("Physical Instance:\n");
                HE_QAT_PRINT("\tpackageId: %d\n", info.physInstId.packageId);
                HE_QAT_PRINT("\tacceleratorId: %d\n",
                             info.physInstId.acceleratorId);
                HE_QAT_PRINT("\texecutionEngineId: %d\n",
                             info.physInstId.executionEngineId);
                HE_QAT_PRINT("\tbusAddress: %d\n", info.physInstId.busAddress);
                HE_QAT_PRINT("\tkptAcHandle: %d\n",
                             info.physInstId.kptAcHandle);
#endif
            }
            HE_QAT_PRINT_DBG("Next Instance: %d.\n", nextInstance);

            if (status == CPA_STATUS_SUCCESS)
                return cyInstHandles[nextInstance];
        }

        if (0 == numInstances) {
            HE_QAT_PRINT_ERR("No instances found for 'SSL'\n");
            HE_QAT_PRINT_ERR("Please check your section names");
            HE_QAT_PRINT_ERR(" in the config file.\n");
            HE_QAT_PRINT_ERR("Also make sure to use config file version 2.\n");
        }

        return NULL;
    }

    nextInstance = ((nextInstance + 1) % numInstances);
    HE_QAT_PRINT_DBG("Next Instance: %d.\n", nextInstance);

    return cyInstHandles[nextInstance];
}

/// @brief
/// Acquire QAT instances and set up QAT execution environment.
HE_QAT_STATUS acquire_qat_devices() {
    CpaStatus status = CPA_STATUS_FAIL;

    pthread_mutex_lock(&context_lock);

    // Handle cases where acquire_qat_devices() is called when already active
    // and running
    if (HE_QAT_STATUS_ACTIVE == context_state) {
        pthread_mutex_unlock(&context_lock);
        return HE_QAT_STATUS_SUCCESS;
    }

    // Initialize QAT memory pool allocator
    status = qaeMemInit();
    if (CPA_STATUS_SUCCESS != status) {
        pthread_mutex_unlock(&context_lock);
        HE_QAT_PRINT_ERR("Failed to initialized memory driver.\n");
        return HE_QAT_STATUS_FAIL;
    }
    HE_QAT_PRINT_DBG("QAT memory successfully initialized.\n");

    status = icp_sal_userStartMultiProcess("SSL", CPA_FALSE);
    if (CPA_STATUS_SUCCESS != status) {
        pthread_mutex_unlock(&context_lock);
        HE_QAT_PRINT_ERR("Failed to start SAL user process SSL\n");
        qaeMemDestroy();
        return HE_QAT_STATUS_FAIL;
    }
    HE_QAT_PRINT_DBG("SAL user process successfully started.\n");

    CpaInstanceHandle _inst_handle[HE_QAT_NUM_ACTIVE_INSTANCES];
    for (unsigned int i = 0; i < HE_QAT_NUM_ACTIVE_INSTANCES; i++) {
        _inst_handle[i] = get_qat_instance();
        if (_inst_handle[i] == NULL) {
            pthread_mutex_unlock(&context_lock);
            HE_QAT_PRINT_ERR("Failed to find QAT endpoints.\n");
            return HE_QAT_STATUS_FAIL;
        }
    }

    HE_QAT_PRINT_DBG("Found QAT endpoints.\n");

    // Initialize QAT buffer synchronization attributes
    he_qat_buffer.count = 0;
    he_qat_buffer.next_free_slot = 0;
    he_qat_buffer.next_data_slot = 0;

    // Initialize QAT memory buffer
    for (int i = 0; i < HE_QAT_BUFFER_SIZE; i++) {
        he_qat_buffer.data[i] = NULL;
    }

    // Initialize QAT outstanding buffers
    outstanding.busy_count = 0;
    outstanding.next_free_buffer = 0;
    outstanding.next_ready_buffer = 0;
    for (int i = 0; i < HE_QAT_BUFFER_COUNT; i++) {
        outstanding.free_buffer[i] = 1;
        outstanding.ready_buffer[i] = 0;
        outstanding.buffer[i].count = 0;
        outstanding.buffer[i].next_free_slot = 0;
        outstanding.buffer[i].next_data_slot = 0;
        outstanding.buffer[i].next_data_out = 0;
        for (int j = 0; j < HE_QAT_BUFFER_SIZE; j++) {
            outstanding.buffer[i].data[j] = NULL;
        }
        pthread_mutex_init(&outstanding.buffer[i].mutex, NULL);
        pthread_cond_init(&outstanding.buffer[i].any_more_data, NULL);
        pthread_cond_init(&outstanding.buffer[i].any_free_slot, NULL);
    }
    pthread_mutex_init(&outstanding.mutex, NULL);
    pthread_cond_init(&outstanding.any_free_buffer, NULL);
    pthread_cond_init(&outstanding.any_ready_buffer, NULL);

    // Creating QAT instances (consumer threads) to process op requests
    cpu_set_t cpus;
    for (int i = 0; i < HE_QAT_NUM_ACTIVE_INSTANCES; i++) {
        CPU_ZERO(&cpus);
        CPU_SET(i, &cpus);
        pthread_attr_init(&he_qat_inst_attr[i]);
        pthread_attr_setaffinity_np(&he_qat_inst_attr[i], sizeof(cpu_set_t),
                                    &cpus);

        // configure thread
        he_qat_inst_config[i].active = 0;   // HE_QAT_STATUS_INACTIVE
        he_qat_inst_config[i].polling = 0;  // HE_QAT_STATUS_INACTIVE
        he_qat_inst_config[i].running = 0;
        he_qat_inst_config[i].status = CPA_STATUS_FAIL;
        pthread_mutex_init(&he_qat_inst_config[i].mutex, NULL);
        pthread_cond_init(&he_qat_inst_config[i].ready, NULL);
        he_qat_inst_config[i].inst_handle = _inst_handle[i];
        he_qat_inst_config[i].inst_id = i;
        he_qat_inst_config[i].attr = &he_qat_inst_attr[i];
    }

    he_qat_config = (HE_QAT_Config*)malloc(  // NOLINT [readability/casting]
        sizeof(HE_QAT_Config));              // NOLINT [readability/casting]
    he_qat_config->inst_config = he_qat_inst_config;
    he_qat_config->count = HE_QAT_NUM_ACTIVE_INSTANCES;
    he_qat_config->running = 0;
    he_qat_config->active = 0;

    pthread_create(&he_qat_runner, NULL, start_instances,
                   (void*)he_qat_config);  // NOLINT [readability/casting]
    HE_QAT_PRINT_DBG("Created processing threads.\n");

    // Dispatch the qat instances to run independently in the background
    pthread_detach(he_qat_runner);
    HE_QAT_PRINT_DBG("Detached processing threads.\n");

    // Set context state to active
    context_state = HE_QAT_STATUS_ACTIVE;

    // Launch buffer manager thread to schedule incoming requests
    if (0 != pthread_create(
                 &buffer_manager, NULL, schedule_requests,
                 (void*)&context_state)) {  // NOLINT [readability/casting]
        pthread_mutex_unlock(&context_lock);
        release_qat_devices();
        HE_QAT_PRINT_ERR(
            "Failed to complete QAT initialization while creating buffer "
            "manager thread.\n");
        return HE_QAT_STATUS_FAIL;
    }

    if (0 != pthread_detach(buffer_manager)) {
        pthread_mutex_unlock(&context_lock);
        release_qat_devices();
        HE_QAT_PRINT_ERR(
            "Failed to complete QAT initialization while launching buffer "
            "manager thread.\n");
        return HE_QAT_STATUS_FAIL;
    }

    pthread_mutex_unlock(&context_lock);

    return HE_QAT_STATUS_SUCCESS;
}

/// @brief
/// Release QAT instances and tear down QAT execution environment.
HE_QAT_STATUS release_qat_devices() {
    pthread_mutex_lock(&context_lock);

    if (HE_QAT_STATUS_INACTIVE == context_state) {
        pthread_mutex_unlock(&context_lock);
        return HE_QAT_STATUS_SUCCESS;
    }

    stop_instances(he_qat_config);
    HE_QAT_PRINT_DBG("Stopped polling and processing threads.\n");

    // Deactivate context (this will terminate buffer manager thread
    context_state = HE_QAT_STATUS_INACTIVE;

    // Stop QAT SSL service
    icp_sal_userStop();
    HE_QAT_PRINT_DBG("Stopped SAL user process.\n");

    // Release QAT allocated memory
    qaeMemDestroy();
    HE_QAT_PRINT_DBG("Release QAT memory.\n");

    numInstances = 0;
    nextInstance = 0;

    pthread_mutex_unlock(&context_lock);

    return HE_QAT_STATUS_SUCCESS;
}

/// @brief  Retrieve and read context state.
/// @return Possible return values are HE_QAT_STATUS_ACTIVE,
///         HE_QAT_STATUS_RUNNING, and HE_QAT_STATUS_INACTIVE.
HE_QAT_STATUS get_qat_context_state() { return context_state; }
