# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# Setup ICP variables
if(DEFINED ENV{ICP_ROOT})
	message(STATUS "Environment variable ICP_ROOT is defined as $ENV{ICP_ROOT}.")
else()
	message(FATAL_ERROR "Environment variable ICP_ROOT must be defined. Try export ICP_ROOT=<QAT_ROOT_PATH>")
endif()

set(ICP_ROOT             $ENV{ICP_ROOT}) 
set(ICP_BUILDOUTPUT_PATH ${ICP_ROOT}/build) 
set(ICP_BUILDSYSTEM_PATH ${ICP_ROOT}/quickassist/build_system) 
set(ICP_API_DIR          ${ICP_ROOT}/quickassist)
set(ICP_LAC_DIR          ${ICP_ROOT}/quickassist/lookaside/access_layer) 
set(ICP_OSAL_DIR         ${ICP_ROOT}/quickassist/utilities/oasl)
set(ICP_ADF_DIR          ${ICP_ROOT}/quickassist/lookaside/access_layer/src/qat_direct) 
set(CMN_ROOT             ${ICP_ROOT}/quickassist/utilities/libusdm_drv)

set(ICP_INC_DIR ${ICP_API_DIR}/include 
                ${ICP_LAC_DIR}/include 
                ${ICP_ADF_DIR}/include
                ${CMN_ROOT}
                ${ICP_API_DIR}/include/dc
                ${ICP_API_DIR}/include/lac)

# Active macros for cpa_sample_utils
add_definitions(-DDO_CRYPTO) 
add_definitions(-DUSER_SPACE) 
add_compile_options(-fPIC)

add_library(libadf_static STATIC IMPORTED GLOBAL)
add_library(libosal_static STATIC IMPORTED GLOBAL)
add_library(libqat_static STATIC IMPORTED GLOBAL)
add_library(libusdm_drv_static STATIC IMPORTED GLOBAL)

set_target_properties(libadf_static PROPERTIES
	IMPORTED_LOCATION ${ICP_BUILDOUTPUT_PATH}/libadf.a
)

set_target_properties(libosal_static PROPERTIES
	IMPORTED_LOCATION ${ICP_BUILDOUTPUT_PATH}/libosal.a
)

set_target_properties(libqat_static PROPERTIES
	IMPORTED_LOCATION ${ICP_BUILDOUTPUT_PATH}/libqat.a
)

set_target_properties(libusdm_drv_static PROPERTIES
	IMPORTED_LOCATION ${ICP_BUILDOUTPUT_PATH}/libusdm_drv.a
)
