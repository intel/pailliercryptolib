// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/utils/util.hpp"

#include <thread>  // NOLINT [build/c++11]

namespace ipcl {

#ifdef IPCL_USE_OMP
const int OMPUtilities::cpus = std::thread::hardware_concurrency();
const int OMPUtilities::nodes = IPCL_NUM_NODES;
const int OMPUtilities::MaxThreads = OMPUtilities::getMaxThreads();
const int OMPUtilities::nodes = OMPUtilities::getNodes();
const int OMPUtilities::cpus = OMPUtilities::getCPUs();
#endif  // IPCL_USE_OMP

}  // namespace ipcl
