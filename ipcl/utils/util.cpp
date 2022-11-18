// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/utils/util.hpp"

#include <thread>  // NOLINT [build/c++11]

namespace ipcl {

#ifdef IPCL_USE_OMP
#ifdef IPCL_RUNTIME_DETECT_CPU_FEATURES
const linuxCPUInfo OMPUtilities::cpuinfo = OMPUtilities::getLinuxCPUInfo();
#endif  // IPCL_RUNTIME_DETECT_CPU_FEATURES
const int OMPUtilities::cpus = std::thread::hardware_concurrency();
const int OMPUtilities::nodes = OMPUtilities::getNodes();
const int OMPUtilities::MaxThreads = OMPUtilities::getMaxThreads();
#endif  // IPCL_USE_OMP

}  // namespace ipcl
