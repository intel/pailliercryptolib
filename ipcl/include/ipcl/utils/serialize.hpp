// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_UTILS_SERIALIZE_HPP_
#define IPCL_INCLUDE_IPCL_UTILS_SERIALIZE_HPP_

#include <cstdlib>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/vector.hpp"

namespace ipcl {

class PublicKey;
class PrivateKey;
class CipherText;

namespace serializer {

template <typename T>
void serialize(std::ostream& ss, const T& obj) {
  cereal::PortableBinaryOutputArchive archive(ss);
  archive(obj);
}

template <typename T>
void deserialize(std::istream& ss, T& obj) {
  cereal::PortableBinaryInputArchive archive(ss);
  archive(obj);
}

template <typename T>
bool serializeToFile(const std::string& fn, const T& obj) {
  std::ofstream ofs(fn, std::ios::out | std::ios::binary);
  if (ofs.is_open()) {
    serializer::serialize(ofs, obj);
    ofs.close();
    return true;
  }
  return false;
}

template <typename T>
bool deserializeFromFile(const std::string& fn, T& obj) {
  std::ifstream ifs(fn, std::ios::in | std::ios::binary);
  if (ifs.is_open()) {
    serializer::deserialize(ifs, obj);
    ifs.close();
    return true;
  }
  return false;
}

class serializerBase {
 public:
  virtual ~serializerBase() {}
  virtual std::string serializedName() const = 0;
};

};  // namespace serializer

}  // namespace ipcl

#endif  // IPCL_INCLUDE_IPCL_UTILS_SERIALIZE_HPP_
