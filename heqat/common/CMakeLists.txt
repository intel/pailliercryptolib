# Include directories
set(COMMON_INC_DIR ${HE_QAT_INC_DIR})
message(STATUS "COMMON_INC_DIR: ${COMMON_INC_DIR}")
message(STATUS "ICP_INC_DIR: ${ICP_INC_DIR}")

# Source files
set(COMMON_SRC ${CMAKE_CURRENT_LIST_DIR}/cpa_sample_utils.c
	       ${CMAKE_CURRENT_LIST_DIR}/utils.c)
list(APPEND HE_QAT_SRC ${COMMON_SRC}) 

set(HE_QAT_SRC ${HE_QAT_SRC} PARENT_SCOPE)

#if(HE_QAT_SHARED)
#  add_library(heqat_common SHARED ${COMMON_SRC})
#else()
#  add_library(heqat_common STATIC ${COMMON_SRC})
#endif()
#
#target_include_directories(heqat_common
#    PRIVATE  $<BUILD_INTERFACE:${COMMON_INC_DIR}>               # Public headers
#    PRIVATE  $<BUILD_INTERFACE:${ICP_INC_DIR}>                  # Public headers
#    PUBLIC  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>   # Public headers
#    PRIVATE ${COMMON_INC_DIR}                                  # Private headers
#    PRIVATE ${ICP_INC_DIR}                                  # Private headers
#)
#
#target_link_libraries(heqat_common PRIVATE udev z)
#
#if(HE_QAT_SHARED)
#  target_link_libraries(heqat_common PRIVATE ${ICP_BUILDOUTPUT_PATH}/libqat_s.so)
#  target_link_libraries(heqat_common PRIVATE ${ICP_BUILDOUTPUT_PATH}/libusdm_drv_s.so)
#else()
#  heqat_create_archive(heqat_common libadf_static)
#  heqat_create_archive(heqat_common libosal_static)
#  heqat_create_archive(heqat_common libqat_static)
#  heqat_create_archive(heqat_common libusdm_drv_static)
#endif()
#
#install(TARGETS heqat_common EXPORT he_qatTargets DESTINATION ${CMAKE_INSTALL_LIBDIR})
#
#set(COMMON_INC_DIR ${COMMON_INC_DIR} PARENT_SCOPE)


