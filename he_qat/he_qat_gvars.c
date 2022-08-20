#include <pthread.h>

#include "he_qat_types.h"
#include "he_qat_gconst.h"

pthread_mutex_t response_mutex;

volatile unsigned long request_count = 0;
volatile unsigned long response_count = 0;

unsigned long request_latency = 0; // unused
unsigned long restart_threshold = NUM_PKE_SLICES; // 48; 

unsigned long max_pending = (NUM_PKE_SLICES * 2 * HE_QAT_NUM_ACTIVE_INSTANCES);

// Each QAT endpoint has 6 PKE slices 
// -- single socket has 4 QAT endpoints (24 simultaneous requests)
// -- dual-socket has 8 QAT endpoints (48 simultaneous requests)
// -- max_pending = (num_sockets * num_qat_devices * num_pke_slices) / k
// -- k (default: 1) can be adjusted dynamically as the measured request_latency deviate from the hardware latency
