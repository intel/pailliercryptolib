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
                            util.cpp
                            common.cpp
)


if(IPCL_SHARED)
	add_library(ipcl SHARED ${IPCL_SRCS})
else()
	add_library(ipcl STATIC ${IPCL_SRCS})
endif()

add_library(IPCL::ipcl ALIAS ipcl)

target_include_directories(ipcl
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	PUBLIC $<BUILD_INTERFACE:${IPCL_INC_DIR}>
	PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)


target_include_directories(ipcl
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	PUBLIC $<BUILD_INTERFACE:${IPPCRYPTO_INC_DIR}>
	PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(IPCL_DETECT_IFMA_RUNTIME)
	target_include_directories(ipcl
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
		PUBLIC $<BUILD_INTERFACE:${CPUFEATURES_INC_DIR}>
		PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	)
	install(DIRECTORY ${CPUFEATURES_INC_DIR}/
		DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}
		FILES_MATCHING
		PATTERN "*.hpp"
		PATTERN "*.h")

endif()

install(DIRECTORY ${IPCL_INC_DIR}/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h")

install(DIRECTORY ${IPPCRYPTO_INC_DIR}/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h")

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(ipcl PUBLIC OpenSSL::SSL OpenSSL::Crypto Threads::Threads -lnuma)

if(IPCL_ENABLE_OMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries(ipcl PUBLIC OpenMP::OpenMP_CXX)
endif()

if(IPCL_SHARED)
	target_link_libraries(ipcl PRIVATE libippcrypto)
	if(IPCL_DETECT_IFMA_RUNTIME)
		target_link_libraries(ipcl PRIVATE libcpu_features)
		target_include_directories(ipcl PRIVATE ${CPUFEATURES_INC_DIR})
	endif()
	target_include_directories(ipcl PRIVATE ${IPPCRYPTO_INC_DIR})
else()
	ipcl_create_archive(ipcl libippcrypto::ippcp)
	ipcl_create_archive(ipcl libippcrypto::crypto_mb)
	if(IPCL_DETECT_IFMA_RUNTIME)
		ipcl_create_archive(ipcl libcpu_features)
		target_include_directories(ipcl PRIVATE ${CPUFEATURES_INC_DIR})
	endif()
	target_include_directories(ipcl PRIVATE ${IPPCRYPTO_INC_DIR})
endif()

set_target_properties(ipcl PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(ipcl PROPERTIES VERSION ${IPCL_VERSION})

if(IPCL_DEBUG)
	set_target_properties(ipcl PROPERTIES OUTPUT_NAME "ipcl_debug")
else()
	set_target_properties(ipcl PROPERTIES OUTPUT_NAME "ipcl")
endif()

install(TARGETS ipcl DESTINATION ${CMAKE_INSTALL_LIBDIR})

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
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

install(FILES ${IPCL_CONFIG_FILENAME}
              ${IPCL_CONFIG_VERSION_FILENAME}
        DESTINATION ${IPCL_CONFIG_INSTALL_DIR})

export(EXPORT IPCLTargets
        FILE ${IPCL_TARGET_FILENAME})
