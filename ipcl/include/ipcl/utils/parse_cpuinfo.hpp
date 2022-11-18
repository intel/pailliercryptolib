// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_UTILS_PARSE_CPUINFO_HPP_
#define IPCL_INCLUDE_IPCL_UTILS_PARSE_CPUINFO_HPP_

#include <algorithm>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>

namespace ipcl {
// trim from start (in place)
static inline void ltrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

static inline void trim(std::string& s) {
  ltrim(s);
  rtrim(s);
}

typedef struct {
  int n_processors = 0;
  int n_cores = 0;
  int n_nodes = 0;
} linuxCPUInfo;

static void parseCPUInfo(linuxCPUInfo& info) {
  std::ifstream cpuinfo;
  cpuinfo.exceptions(std::ifstream::badbit);

  try {
    cpuinfo.open("/proc/cpuinfo", std::ios::in);
    std::string line;
    while (std::getline(cpuinfo, line)) {
      std::stringstream ss(line);
      std::string key, val;
      if (std::getline(ss, key, ':') && std::getline(ss, val)) {
        trim(key);
        trim(val);
        if (key == "processor")
          info.n_processors++;
        else if (key == "core id")
          info.n_cores = std::max(info.n_cores, std::stoi(val));
        else if (key == "physical id")
          info.n_nodes = std::max(info.n_nodes, std::stoi(val));
      }
    }
    info.n_nodes++;
    info.n_cores = (info.n_cores + 1) * info.n_nodes;
  } catch (const std::ifstream::failure& e) {
    std::ostringstream log;
    log << "\nFile: " << __FILE__ << "\nLine: " << __LINE__ << "\nError: "
        << "cannot parse /proc/cpuinfo";
    throw std::runtime_error(log.str());
  }
}
linuxCPUInfo GetLinuxCPUInfo(void);

}  // namespace ipcl

#endif  // IPCL_INCLUDE_IPCL_UTILS_PARSE_CPUINFO_HPP_
