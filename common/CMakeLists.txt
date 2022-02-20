
# Include directories
list(APPEND COMMON_INC_DIR  
            ${ICP_API_DIR}/include/dc
            ${ICP_API_DIR}/include/lac
)
message(STATUS "COMMON_INC_DIR: ${COMMON_INC_DIR}")

# Source files
set(COMMON_SRC ${CMAKE_CURRENT_LIST_DIR}/cpa_sample_utils.c)

add_library(cpa_sample_utils ${COMMON_SRC})
target_include_directories(cpa_sample_utils
    PUBLIC  $<BUILD_INTERFACE:${COMMON_INC_DIR}>               # Public headers
    PUBLIC  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>   # Public headers
    PRIVATE ${COMMON_INC_DIR}                                  # Private headers
)
target_link_libraries(cpa_sample_utils PRIVATE udev pthread crypto z)
target_link_libraries(cpa_sample_utils PUBLIC ${ICP_BUILDOUTPUT_PATH}/libqat_s.so)
target_link_libraries(cpa_sample_utils PUBLIC ${ICP_BUILDOUTPUT_PATH}/libusdm_drv_s.so)

set(COMMON_INC_DIR ${COMMON_INC_DIR} PARENT_SCOPE)

