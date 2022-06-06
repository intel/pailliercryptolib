// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/util.hpp"

#ifdef IPCL_USE_OMP
#include <omp.h>
#endif  // IPCL_USE_OMP

namespace ipcl {

#ifdef IPCL_USE_OMP
const int OMPUtilities::MaxThreads = omp_get_max_threads();
#endif  // IPCL_USE_OMP

}  // namespace ipcl
