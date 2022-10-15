// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ipcl/context.hpp"

#include <map>
#include <string>

#ifdef IPCL_USE_QAT
#include <he_qat_context.h>
#endif

namespace ipcl {

  ///>  Default behavior is selected at runtime and implementation dependent
  enum class RuntimeValue { DEFAULT, CPU, QAT, HYBRID };
  std::map<std::string, RuntimeValue> runtimeMap = {
    {"DEFAULT",RuntimeValue::DEFAULT}, {"default",RuntimeValue::DEFAULT},
    {"CPU",RuntimeValue::CPU}, {"cpu",RuntimeValue::CPU},
    {"QAT",RuntimeValue::QAT}, {"qat",RuntimeValue::QAT},
    {"HYBRID",RuntimeValue::HYBRID}, {"hybrid",RuntimeValue::HYBRID}
  };

  enum class FeatureValue { AVX512IFMA, QAT4XXX };
  std::map<std::string, FeatureValue> hasFeatureMap = { 
    {"avx512",FeatureValue::AVX512IFMA}, {"avx512ifma",FeatureValue::AVX512IFMA},
    {"4xxx",FeatureValue::QAT4XXX}, {"qat_4xxx",FeatureValue::QAT4XXX} 
  };

#ifdef IPCL_USE_QAT
  bool hasQAT = false;
  static bool isUsingQAT = false;
  static bool initializeQATContext() {
    if (!isUsingQAT && 
		    HE_QAT_STATUS_SUCCESS == acquire_qat_devices())
      return (isUsingQAT = true);
    return false;
  }
#endif

  bool initializeContext(std::string runtime_choice) {   
#ifdef IPCL_USE_QAT
    hasQAT = true;
    switch (runtimeMap[runtime_choice]) {
      case RuntimeValue::QAT:
	   return initializeQATContext();
      case RuntimeValue::CPU:
      case RuntimeValue::HYBRID:
      case RuntimeValue::DEFAULT:
      default:
         return true;	      
    } 
#else // Default behavior: CPU choice
    return true;
#endif  // IPCL_USE_QAT
  }

  bool terminateContext() {  
#ifdef IPCL_USE_QAT
    if (isUsingQAT) {
      if (HE_QAT_STATUS_SUCCESS == release_qat_devices()) { 
         isUsingQAT = false;
         return true;
      }
      return false;
    } return true;
#else // Default behavior: CPU choice
    return true;
#endif  // IPCL_USE_QAT
  }

  bool isQATRunning() {
#ifdef IPCL_USE_QAT
    return (HE_QAT_STATUS_RUNNING == get_qat_context_state());
#else
    return false;
#endif
  }
  
  bool isQATActive() {
#ifdef IPCL_USE_QAT
    return (HE_QAT_STATUS_ACTIVE == get_qat_context_state());
#else
    return false;
#endif
  }

}  // namespace ipcl
