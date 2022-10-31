# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# HE QAT Lib source code
set(HE_QAT_SRC ${HE_QAT_SRC_DIR}/cb.c
	       ${HE_QAT_SRC_DIR}/context.c
	       ${HE_QAT_SRC_DIR}/ctrl.c
	       ${HE_QAT_SRC_DIR}/bnops.c
         ${HE_QAT_SRC_DIR}/common/utils.c
)

# Helper functions for ippcrypto's BigNumber class
if(HE_QAT_MISC)
  list(APPEND HE_QAT_SRC          ${HE_QAT_SRC_DIR}/misc/misc.cpp
  ${HE_QAT_SRC_DIR}/misc/utils.cpp
  ${HE_QAT_SRC_DIR}/misc/bignum.cpp
)
endif()

if(HE_QAT_SHARED)
  add_library(he_qat SHARED ${HE_QAT_SRC})
else()
  add_library(he_qat STATIC ${HE_QAT_SRC})
endif()

add_library(HE_QAT::he_qat ALIAS he_qat)

target_include_directories(he_qat
	PUBLIC $<BUILD_INTERFACE:${HE_QAT_INC_DIR}> #Public headers
	PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> #Public headers
	PUBLIC ${ICP_INC_DIR}
)

install(DIRECTORY ${HE_QAT_INC_DIR}/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING
	PATTERN "*.hpp"
	PATTERN "*.h")

target_link_directories(he_qat PUBLIC ${ICP_BUILDOUTPUT_PATH})

target_link_libraries(he_qat PRIVATE udev z)
target_link_libraries(he_qat PRIVATE OpenSSL::SSL)
target_link_libraries(he_qat PRIVATE Threads::Threads)
if(HE_QAT_SHARED)
  target_link_libraries(he_qat PRIVATE qat_s)
  target_link_libraries(he_qat PRIVATE usdm_drv_s)
else()
  heqat_create_archive(he_qat libadf_static)
  heqat_create_archive(he_qat libosal_static)
  heqat_create_archive(he_qat libqat_static)
  heqat_create_archive(he_qat libusdm_drv_static)
endif()

if(NOT HE_QAT_STANDALONE)
  add_dependencies(he_qat IPPCP::ippcp)
endif()

if(HE_QAT_MISC)
  target_include_directories(he_qat PRIVATE ${IPPCP_INC_DIR})
  target_link_directories(he_qat PRIVATE ${IPPCP_LIB_DIR})
  if(HE_QAT_SHARED)
    target_link_libraries(he_qat PRIVATE IPPCP::ippcp)
  else()
    heqat_create_archive(he_qat IPPCP::ippcp)
  endif()
endif()

set_target_properties(he_qat PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(he_qat PROPERTIES VERSION ${HE_QAT_VERSION})

if(HE_QAT_DEBUG)
  set_target_properties(he_qat PROPERTIES OUTPUT_NAME "he_qat_debug")
else()
  set_target_properties(he_qat PROPERTIES OUTPUT_NAME "he_qat")
endif()

include(CMakePackageConfigHelpers)

# config cmake config and target file
set(HE_QAT_TARGET_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/he_qat-${HE_QAT_VERSION}/he_qatTargets.cmake)
set(HE_QAT_CONFIG_IN_FILENAME ${HE_QAT_CMAKE_PATH}/HE_QATConfig.cmake.in)
set(HE_QAT_CONFIG_FILENAME ${HE_QAT_ROOT_DIR}/cmake/he_qat-${HE_QAT_VERSION}/HE_QATConfig.cmake)
set(HE_QAT_CONFIG_VERSION_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/he_qat-${HE_QAT_VERSION}/HE_QATConfigVersion.cmake)
set(HE_QAT_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/he_qat-${HE_QAT_VERSION}/)

install(
	EXPORT he_qatTargets
	NAMESPACE HE_QAT::
	DESTINATION ${HE_QAT_CONFIG_INSTALL_DIR}
)

write_basic_package_version_file(
    ${HE_QAT_CONFIG_VERSION_FILENAME}
    VERSION ${HE_QAT_VERSION}
    COMPATIBILITY ExactVersion
)

configure_package_config_file(
		${HE_QAT_CONFIG_IN_FILENAME} ${HE_QAT_CONFIG_FILENAME}
		INSTALL_DESTINATION ${HE_QAT_CONFIG_INSTALL_DIR}
)

install(
    TARGETS he_qat
    EXPORT he_qatTargets
    ARCHIVE DESTINATION ${HE_QAT_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${HE_QAT_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${HE_QAT_INSTALL_LIBDIR}
    )

install(FILES ${HE_QAT_CONFIG_FILENAME}
              ${HE_QAT_CONFIG_VERSION_FILENAME}
        DESTINATION ${HE_QAT_CONFIG_INSTALL_DIR})

export(EXPORT he_qatTargets
        FILE ${HE_QAT_TARGET_FILENAME})
