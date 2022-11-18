// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/utils/util.hpp"

namespace ipcl {

#ifdef IPCL_USE_OMP
const int OMPUtilities::MaxThreads = OMPUtilities::getMaxThreads();
const int OMPUtilities::nodes = OMPUtilities::getNodes();
const int OMPUtilities::cpus = OMPUtilities::getCPUs();
#endif  // IPCL_USE_OMP

}  // namespace ipcl
