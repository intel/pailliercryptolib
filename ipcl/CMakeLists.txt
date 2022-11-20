# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

set(IPCL_SRCS pri_key.cpp
              pub_key.cpp
              keygen.cpp
              bignum.cpp
              mod_exp.cpp
              base_text.cpp
              plaintext.cpp
              ciphertext.cpp
              utils/context.cpp
              utils/util.cpp
              utils/common.cpp
              utils/parse_cpuinfo.cpp
)

if(IPCL_SHARED)
	add_library(ipcl SHARED ${IPCL_SRCS})
else()
	add_library(ipcl STATIC ${IPCL_SRCS})
endif()

add_library(IPCL::ipcl ALIAS ipcl)

set_target_properties(ipcl PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    LINK_FLAGS "-Wl,-rpath,'$ORIGIN' -Wl,-rpath,'$ORIGIN'/ippcrypto -Wl,-rpath,'$ORIGIN'/cpufeatures")

target_include_directories(ipcl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# include and install definition of IPCL
target_include_directories(ipcl
	PUBLIC $<BUILD_INTERFACE:${IPCL_INC_DIR}>
	PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ipcl>
)

install(DIRECTORY ${IPCL_INC_DIR}/
	DESTINATION ${IPCL_INSTALL_INCLUDEDIR}
	FILES_MATCHING
	PATTERN "*.hpp"
	PATTERN "*.h"
)

# CEREAL (third party dep): include and install definition
target_include_directories(ipcl
    PUBLIC $<BUILD_INTERFACE:${CEREAL_INC_DIR}>
	PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ipcl>
)

install(DIRECTORY ${CEREAL_INC_DIR}/
    DESTINATION ${IPCL_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.h"
)

# IPP-Crypto (third party dep): include and install definition
target_include_directories(ipcl
    PUBLIC $<BUILD_INTERFACE:${IPPCRYPTO_INC_DIR}/ippcrypto>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ipcl/ippcrypto>
)

install(DIRECTORY ${IPPCRYPTO_INC_DIR}/
    DESTINATION ${IPCL_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.h"
)

# include and install definition of cpu_features
if(IPCL_DETECT_CPU_RUNTIME)
	target_include_directories(ipcl
        PUBLIC $<BUILD_INTERFACE:${CPUFEATURES_INC_DIR}>
		PRIVATE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ipcl>
	)
	install(DIRECTORY ${CPUFEATURES_INC_DIR}/
		DESTINATION ${IPCL_INSTALL_INCLUDEDIR}
		FILES_MATCHING
		PATTERN "*.hpp"
		PATTERN "*.h"
    )
endif()

# include and install definition of he_qat
if(IPCL_ENABLE_QAT)
    ipcl_define_icp_variables(icp_inc_dir)
    target_include_directories(ipcl
        PRIVATE "$<BUILD_INTERFACE:${icp_inc_dir}>"
    )

    target_include_directories(ipcl
        PRIVATE "$<BUILD_INTERFACE:${HE_QAT_INC_DIR}>"
        PRIVATE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDE_DIR}/heqat>
    )
endif()

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(ipcl PUBLIC OpenSSL::SSL OpenSSL::Crypto Threads::Threads)

if(IPCL_ENABLE_OMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries(ipcl PUBLIC OpenMP::OpenMP_CXX)
endif()

if(IPCL_SHARED)
	target_link_libraries(ipcl PRIVATE IPPCP::ippcp IPPCP::crypto_mb)
	if(IPCL_DETECT_CPU_RUNTIME)
        target_link_libraries(ipcl PRIVATE libcpu_features)
	endif()
	if(IPCL_ENABLE_QAT)
	    target_link_libraries(ipcl PRIVATE he_qat udev z)
	endif()
else()
    ipcl_create_archive(ipcl IPPCP::crypto_mb)
    ipcl_create_archive(ipcl IPPCP::ippcp)
	if(IPCL_ENABLE_QAT)
        ipcl_create_archive(ipcl he_qat)
        target_link_libraries(ipcl PRIVATE udev z)
    endif()

	if(IPCL_DETECT_CPU_RUNTIME)
        ipcl_create_archive(ipcl libcpu_features)
	endif()
endif()

set_target_properties(ipcl PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(ipcl PROPERTIES VERSION ${IPCL_VERSION})
if(IPCL_DEBUG)
	set_target_properties(ipcl PROPERTIES OUTPUT_NAME "ipcl_debug")
else()
	set_target_properties(ipcl PROPERTIES OUTPUT_NAME "ipcl")
endif()

# config cmake config and target file
set(IPCL_TARGET_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/ipcl-${IPCL_VERSION}/IPCLTargets.cmake)
set(IPCL_CONFIG_IN_FILENAME ${IPCL_CMAKE_PATH}/IPCLConfig.cmake.in)
set(IPCL_CONFIG_FILENAME ${IPCL_ROOT_DIR}/cmake/ipcl-${IPCL_VERSION}/IPCLConfig.cmake)
set(IPCL_CONFIG_VERSION_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/cmake/ipcl-${IPCL_VERSION}/IPCLConfigVersion.cmake)
set(IPCL_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/ipcl-${IPCL_VERSION}/)

install(
	EXPORT IPCLTargets
	NAMESPACE IPCL::
	DESTINATION ${IPCL_CONFIG_INSTALL_DIR}
)

write_basic_package_version_file(
		${IPCL_CONFIG_VERSION_FILENAME}
		VERSION ${IPCL_VERSION}
		COMPATIBILITY ExactVersion
)

configure_package_config_file(
		${IPCL_CONFIG_IN_FILENAME} ${IPCL_CONFIG_FILENAME}
		INSTALL_DESTINATION ${IPCL_CONFIG_INSTALL_DIR}
)

install(
    TARGETS ipcl
    EXPORT IPCLTargets
    ARCHIVE DESTINATION ${IPCL_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${IPCL_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${IPCL_INSTALL_LIBDIR}
    )

install(FILES ${IPCL_CONFIG_FILENAME}
              ${IPCL_CONFIG_VERSION_FILENAME}
        DESTINATION ${IPCL_CONFIG_INSTALL_DIR})

export(EXPORT IPCLTargets
        FILE ${IPCL_TARGET_FILENAME})
