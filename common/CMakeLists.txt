# Include directories
#list(APPEND COMMON_INC_DIR  
#            ${ICP_API_DIR}/include/dc
#            ${ICP_API_DIR}/include/lac
#)
message(STATUS "COMMON_INC_DIR: ${COMMON_INC_DIR}")
message(STATUS "ICP_INC_DIR: ${ICP_INC_DIR}")

# Source files
set(COMMON_SRC ${CMAKE_CURRENT_LIST_DIR}/cpa_sample_utils.c)

if(HE_QAT_SHARED)
  add_library(cpa_sample_utils SHARED ${COMMON_SRC})
else()
  add_library(cpa_sample_utils STATIC ${COMMON_SRC})
endif()

target_include_directories(cpa_sample_utils
    PRIVATE  $<BUILD_INTERFACE:${COMMON_INC_DIR}>               # Public headers
    PRIVATE  $<BUILD_INTERFACE:${ICP_INC_DIR}>                  # Public headers
    PUBLIC  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>   # Public headers
    PRIVATE ${COMMON_INC_DIR}                                  # Private headers
    PRIVATE ${ICP_INC_DIR}                                  # Private headers
)

target_link_libraries(cpa_sample_utils PRIVATE udev z)

if(HE_QAT_SHARED)
  target_link_libraries(cpa_sample_utils PRIVATE ${ICP_BUILDOUTPUT_PATH}/libqat_s.so)
  target_link_libraries(cpa_sample_utils PRIVATE ${ICP_BUILDOUTPUT_PATH}/libusdm_drv_s.so)
else()
  heqat_create_archive(cpa_sample_utils libadf_static)
  heqat_create_archive(cpa_sample_utils libosal_static)
  heqat_create_archive(cpa_sample_utils libqat_static)
  heqat_create_archive(cpa_sample_utils libusdm_drv_static)
endif()

#install(TARGETS cpa_sample_utils DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS cpa_sample_utils EXPORT he_qatTargets DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(COMMON_INC_DIR ${COMMON_INC_DIR} PARENT_SCOPE)


