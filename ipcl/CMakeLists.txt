# Copyright (C) 2021-2022 Intel Corporation

# # BigNumber library
# add_library(bignum SHARED
#   bignum.cpp)
# target_include_directories(bignum
# 					PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
# 					PUBLIC ${IPCL_INC_DIR}
# )
# target_link_libraries(bignum PRIVATE libippcrypto Threads::Threads)


# Ipp-crypto Paillier Library

set(IPCL_SRCS paillier_pri.cpp
							paillier_pub.cpp
							paillier_ops.cpp
							key_pair.cpp
							bignum.cpp
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

install(DIRECTORY ${IPCL_INC_DIR}/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/
        FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h")

if(IPCL_SHARED)
	target_link_libraries(ipcl PRIVATE libippcrypto Threads::Threads)
else()
	target_link_libraries(ipcl PRIVATE Threads::Threads)
	target_link_libraries(ipcl PRIVATE OpenSSL::SSL OpenSSL::Crypto)
	ipcl_create_archive(ipcl libippcrypto::ippcp)
	ipcl_create_archive(ipcl libippcrypto::crypto_mb)
	target_include_directories(ipcl PRIVATE ${IPPCRYPTO_INC_DIR})
endif()


set_target_properties(ipcl PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(ipcl PROPERTIES VERSION ${IPCL_VERSION})
if(IPCL_DEBUG)
	set_target_properties(ipcl PROPERTIES OUTPUT_NAME "ipcl_debug")
else()
	set_target_properties(ipcl PROPERTIES OUTPUT_NAME "ipcl")
endif()


install(DIRECTORY ${IPCL_INC_DIR}/
				DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/
				FILES_MATCHING
				PATTERN "*.hpp"
				PATTERN "*.h"
)

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
