
#pragma once

#include "he_qat_types.h"

const int HE_QAT_MAX_NUM_INST 8

/// @brief 
/// @function acquire_qat_devices
/// Configure and initialize QAT runtime environment.
HE_QAT_STATUS acquire_qat_devices();

/// @brief 
/// @function release_qat_devices
/// Configure and initialize QAT runtime environment.
HE_QAT_STATUS release_qat_devices();

