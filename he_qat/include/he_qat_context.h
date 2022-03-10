
#pragma once

#ifndef _HE_QAT_CONTEXT_H_
#define _HE_QAT_CONTEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "he_qat_types.h"
#include <stdlib.h>

#define HE_QAT_MAX_NUM_INST 8
// const unsigned HE_QAT_MAX_NUM_INST = 8

/// @brief
/// @function acquire_qat_devices
/// Configure and initialize QAT runtime environment.
HE_QAT_STATUS acquire_qat_devices();

/// @brief
/// @function release_qat_devices
/// Configure and initialize QAT runtime environment.
HE_QAT_STATUS release_qat_devices();

#ifdef __cplusplus
}  // extern "C" {
#endif

#endif
