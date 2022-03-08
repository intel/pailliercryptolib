// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_UTIL_HPP_
#define IPCL_INCLUDE_IPCL_UTIL_HPP_

#include <exception>
#include <string>
#include <vector>

#include "ipcl/common.hpp"

namespace ipcl {

static inline std::string build_log(const char* file, int line,
                                    std::string msg) {
  std::string log;
  log = "\nFile: " + std::string(file);
  log += "\nLine: " + std::to_string(line);
  log += "\nError: " + msg;

  return log;
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

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_UTIL_HPP_
