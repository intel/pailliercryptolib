
set(HE_QAT_SRC ${HE_QAT_SRC_DIR}/he_qat_cb.c
	       ${HE_QAT_SRC_DIR}/he_qat_bn_ops.c 
	       ${HE_QAT_SRC_DIR}/he_qat_context.c
	       ${HE_QAT_SRC_DIR}/he_qat_utils.c
)

if(HE_QAT_SHARED)
	add_library(he_qat SHARED ${HE_QAT_SRC})
else()
	add_library(he_qat STATIC ${HE_QAT_SRC})
endif()

add_library(HE_QAT::he_qat ALIAS he_qat)
#add_library(he_qat SHARED ${HE_QAT_SRC})

target_include_directories(he_qat 
	PUBLIC $<BUILD_INTERFACE:${COMMON_INC_DIR}> #Public headers 
	PUBLIC $<BUILD_INTERFACE:${HE_QAT_INC_DIR}> #Public headers 
	PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> #Public headers
	PRIVATE ${COMMON_INC_DIR} #Private headers 
	PRIVATE ${ICP_INC_DIR} #Private headers 
)

install(DIRECTORY ${COMMON_INC_DIR}/
	DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/
	FILES_MATCHING 
	PATTERN "*.hpp"
	PATTERN "*.h")

install(DIRECTORY ${HE_QAT_INC_DIR}/
	DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/
	FILES_MATCHING 
	PATTERN "*.hpp"
	PATTERN "*.h")

target_link_directories(he_qat PUBLIC ${ICP_BUILDOUTPUT_PATH})
target_link_libraries(he_qat PRIVATE OpenSSL::SSL Threads::Threads)
target_link_libraries(he_qat PRIVATE udev z)

if(HE_QAT_MISC)
	target_link_libraries(he_qat PRIVATE he_qat_misc)
endif()

if(HE_QAT_SHARED)
	target_link_libraries(he_qat PRIVATE qat_s usdm_drv_s)
	target_link_libraries(he_qat PRIVATE cpa_sample_utils)
else()
	heqat_create_archive(he_qat cpa_sample_utils)
endif()

set_target_properties(he_qat PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(he_qat PROPERTIES VERSION ${HE_QAT_VERSION})

if(HE_QAT_DEBUG)
	set_target_properties(he_qat PROPERTIES OUTPUT_NAME "he_qat_debug")
else()
	set_target_properties(he_qat PROPERTIES OUTPUT_NAME "he_qat")
endif()


install(TARGETS he_qat DESTINATION ${CMAKE_INSTALL_LIBDIR})

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
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

install(FILES ${HE_QAT_CONFIG_FILENAME}
              ${HE_QAT_CONFIG_VERSION_FILENAME}
        DESTINATION ${HE_QAT_CONFIG_INSTALL_DIR})

export(EXPORT he_qatTargets
        FILE ${HE_QAT_TARGET_FILENAME})

