// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/util.hpp"

#include <omp.h>

namespace ipcl {

const int OMPUtilities::MaxThreads = omp_get_max_threads();

}  // namespace ipcl
