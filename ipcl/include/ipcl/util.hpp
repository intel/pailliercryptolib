// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_UTIL_HPP_
#define IPCL_INCLUDE_IPCL_UTIL_HPP_

#ifdef IPCL_RUNTIME_MOD_EXP
#include <cpu_features/cpuinfo_x86.h>
#endif  // IPCL_RUNTIME_MOD_EXP

#include <cstdlib>
#include <exception>
#include <sstream>
#include <string>
#include <vector>

#include "ipcl/common.hpp"

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
  static const int nodes;
  static const int cpus;

  static int getMaxThreads() {
#ifdef IPCL_NUM_THREADS
    return IPCL_NUM_THREADS;
#else
    return cpus / nodes;
#endif  // IPCL_NUM_THREADS
  }
};

#endif  // IPCL_USE_OMP

#ifdef IPCL_RUNTIME_MOD_EXP
static const bool disable_avx512ifma =
    (std::getenv("IPCL_DISABLE_AVX512IFMA") != nullptr);
static const cpu_features::X86Features features =
    cpu_features::GetX86Info().features;
static const bool has_avx512ifma = features.avx512ifma && !disable_avx512ifma;
#endif  // IPCL_RUNTIME_MOD_EXP

}  // namespace ipcl

#endif  // IPCL_INCLUDE_IPCL_UTIL_HPP_
