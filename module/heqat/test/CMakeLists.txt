# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
###############################################################################

macro(heqat_create_executable test_case language dependencies)
  if(${language} STREQUAL "C" OR ${language} STREQUAL "c")
    set(extension "c")
  elseif(${language} STREQUAL "CXX" OR ${language} STREQUAL "cxx")
    set(extension "cpp")
  else()
    message(FATAL_ERROR "Error language not supported. Options: C or CXX.")
  endif()

  set(target test_${test_case})
  add_executable(${target} test_${test_case}.${extension})
  
  target_include_directories(${target} PUBLIC ${HE_QAT_INC_DIR})
  
  target_link_libraries(${target} PUBLIC he_qat)
  
  if(NOT ${dependencies} STREQUAL "")
    message(STATUS "Target: ${target} Additional Dependencies: ${${dependencies}}")
    target_link_libraries(${target} PUBLIC ${${dependencies}})
  endif()

  install(TARGETS ${target} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endmacro()

##############################################################################

set(EXECUTABLE_DEPENDENCIES OpenSSL::SSL)

# Sample testing the robustness of the heqatlib context functions
heqat_create_executable(context c "")

# Sample demonstrating how to use API for BIGNUM inputs
heqat_create_executable(BIGNUMModExp C EXECUTABLE_DEPENDENCIES)

if(HE_QAT_MISC)
  add_compile_options(-fpermissive)

  list(APPEND EXECUTABLE_DEPENDENCIES IPPCP::ippcp)
  
  # Sample showing how to convert from/to BigNumber to/from CpaFlatBuffer
  heqat_create_executable(bnConversion cxx EXECUTABLE_DEPENDENCIES)

  # Sample showing how to use bnModExp API
  heqat_create_executable(bnModExp CXX EXECUTABLE_DEPENDENCIES) 

  if(OpenMP_CXX_FOUND) 
    list(APPEND EXECUTABLE_DEPENDENCIES OpenMP::OpenMP_CXX)
    # Sample showing how to use bnModExp_MT API for multithreaded applications
    heqat_create_executable(bnModExp_MT CXX EXECUTABLE_DEPENDENCIES) 
  endif()
endif()

###############################################################################
