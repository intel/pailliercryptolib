// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/util.hpp"

#ifdef IPCL_USE_OMP
#include <numa.h>
#endif  // IPCL_USE_OMP

namespace ipcl {

#ifdef IPCL_USE_OMP
const int OMPUtilities::nodes = numa_num_configured_nodes();
const int OMPUtilities::cpus = numa_num_configured_cpus();
const int OMPUtilities::MaxThreads = OMPUtilities::getMaxThreads();

#endif  // IPCL_USE_OMP

}  // namespace ipcl
