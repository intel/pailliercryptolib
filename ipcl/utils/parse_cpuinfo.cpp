// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/utils/parse_cpuinfo.hpp"

#include <fstream>
#include <sstream>

ipcl::linuxCPUInfo ipcl::getLinuxCPUInfoImpl(void) {
  ipcl::linuxCPUInfo info;
  ipcl::parseCPUInfo(info);
  return info;
}
