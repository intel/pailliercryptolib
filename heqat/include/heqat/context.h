/// @file heqat/context.h

#pragma once

#ifndef _HE_QAT_CONTEXT_H_
#define _HE_QAT_CONTEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "heqat/common/types.h"

/// @brief
/// Configure and initialize QAT runtime environment.
HE_QAT_STATUS acquire_qat_devices();

/// @brief
/// Release and free resources of the QAT runtime environment.
HE_QAT_STATUS release_qat_devices();

/// @brief
/// Probe context status of the QAT runtime environment.
HE_QAT_STATUS get_qat_context_state();

#ifdef __cplusplus
}  // extern "C" {
#endif

#endif
