// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_UTILS_UTIL_HPP_
#define IPCL_INCLUDE_IPCL_UTILS_UTIL_HPP_

#ifdef IPCL_RUNTIME_DETECT_CPU_FEATURES
#include <cpu_features/cpuinfo_x86.h>

#include "ipcl/utils/parse_cpuinfo.hpp"
#endif  // IPCL_RUNTIME_DETECT_CPU_FEATURES

#include <cstdlib>
#include <exception>
#include <sstream>
#include <string>
#include <vector>

#include "ipcl/utils/common.hpp"

namespace ipcl {

inline std::string build_log(const char* file, int line,
                             const std::string& msg) {
  std::ostringstream log;
  log << "\nFile: " << file << "\nLine: " << line << "\nError: " << msg;
  return log.str();
}

#define ERROR_CHECK(e, ...)                                                 \
  do {                                                                      \
    if (!(e))                                                               \
      throw std::runtime_error(build_log(__FILE__, __LINE__, __VA_ARGS__)); \
  } while (0)

template <typename T>
inline void vec_size_check(const std::vector<T>& v, const char* file,
                           int line) {
  if (v.size() != IPCL_CRYPTO_MB_SIZE)
    throw std::runtime_error(build_log(
        file, line, "Vector size is NOT equal to IPCL_CRYPTO_MB_SIZE"));
}

#define VEC_SIZE_CHECK(v) vec_size_check(v, __FILE__, __LINE__)

#ifdef IPCL_RUNTIME_DETECT_CPU_FEATURES
static const bool disable_avx512ifma =
    (std::getenv("IPCL_DISABLE_AVX512IFMA") != nullptr);
static const bool prefer_rdrand =
    (std::getenv("IPCL_PREFER_RDRAND") != nullptr);
static const bool prefer_ipp_prng =
    (std::getenv("IPCL_PREFER_IPP_PRNG") != nullptr);
static const cpu_features::X86Features features =
    cpu_features::GetX86Info().features;
static const bool has_avx512ifma = features.avx512ifma && !disable_avx512ifma;
static const bool has_rdseed =
    features.rdseed && !prefer_rdrand && !prefer_ipp_prng;
static const bool has_rdrand = features.rdrnd && prefer_rdrand;

#endif  // IPCL_RUNTIME_DETECT_CPU_FEATURES

#ifdef IPCL_USE_OMP
class OMPUtilities {
 public:
  static const int MaxThreads;

  static int assignOMPThreads(int& remaining_threads, int requested_threads) {
    int retval = (requested_threads > 0 ? requested_threads : 1);
    if (retval > remaining_threads) retval = remaining_threads;
    if (retval > 1)
      remaining_threads -= retval;
    else
      retval = 1;
    return retval;
  }

 private:
#ifdef IPCL_RUNTIME_DETECT_CPU_FEATURES
  static const linuxCPUInfo cpuinfo;
  static const linuxCPUInfo getLinuxCPUInfo() { return GetLinuxCPUInfo(); }
#endif
  static const int nodes;
  static const int cpus;

  static int getNodes() {
#ifdef IPCL_RUNTIME_DETECT_CPU_FEATURES
    return cpuinfo.n_nodes;
#else
    return IPCL_NUM_NODES;
#endif  // IPCL_RUNTIME_DETECT_CPU_FEATURES
  }
  static int getMaxThreads() {
#ifdef IPCL_NUM_THREADS
    return IPCL_NUM_THREADS;
#else
    return cpus / nodes;
#endif  // IPCL_NUM_THREADS
  }
};

#endif  // IPCL_USE_OMP

}  // namespace ipcl

#endif  // IPCL_INCLUDE_IPCL_UTILS_UTIL_HPP_
