// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifndef IPCL_INCLUDE_IPCL_CONTEXT_HPP_
#define IPCL_INCLUDE_IPCL_CONTEXT_HPP_

#include <string>

namespace ipcl {

/**
 * Initialize device (CPU, QAT, or both) runtime context for the Paillier crypto
 * services.
 * @details It must be called if there is intent of using QAT devices for
 * compute acceleration.
 * @param[in] runtime_choice Acceptable values are "CPU", "cpu", "QAT", "qat",
 * "HYBRID", "hybrid", "DEFAULT", "default". Anything other than the accepted
 * values, including typos and absence thereof, will default to the "DEFAULT"
 * runtime choice.
 * @return true if runtime context has been properly initialized, false
 * otherwise.
 */
bool initializeContext(const std::string runtime_choice);

/**
 * Terminate runtime context.
 * @return true if runtime context has been properly terminated, false
 * otherwise.
 */
bool terminateContext(void);

/**
 * Determine if QAT instances are running for IPCL.
 * @return true if QAT instances are active and running, false otherwise.
 */
bool isQATRunning(void);

/**
 * Determine if QAT instances are active for IPCL.
 * @return true if QAT instances are active, false otherwise.
 */
bool isQATActive(void);

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_CONTEXT_HPP_
