cmake_minimum_required(VERSION 3.13)

project(QATModExp LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CMakePackageConfigHelpers)

set(COMMON_INC_DIR ${CMAKE_CURRENT_LIST_DIR}/include)

set(CMAKE_INSTALL_PREFIX "install") 

set(ICP_ROOT $ENV{ICP_ROOT})
set(ICP_BUILDOUTPUT_PATH ${ICP_ROOT}/build)
set(ICP_BUILDSYSTEM_PATH ${ICP_ROOT}/quickassist/build_system)
set(ICP_API_DIR ${ICP_ROOT}/quickassist)
set(ICP_LAC_DIR ${ICP_ROOT}/quickassist/lookaside/access_layer)
set(ICP_OSAL_DIR ${ICP_ROOT}/quickassist/utilities/oasl)
set(ICP_ADF_DIR ${ICP_ROOT}/quickassist/lookaside/access_layer/src/qat_direct)
set(CMN_ROOT ${ICP_ROOT}/quickassist/utilities/libusdm_drv)

list(APPEND COMMON_INC_DIR 
    ${ICP_API_DIR}/include
    ${ICP_LAC_DIR}/include  
    ${ICP_ADF_DIR}/include
    ${CMN_ROOT} 
)

# Macros for the test case
add_definitions(-DDO_CRYPTO)
add_definitions(-DUSER_SPACE)
add_compile_options(-fPIC)


# Common utility functions
add_subdirectory(common)

set(HE_QAT_SRC ${CMAKE_CURRENT_LIST_DIR}/he_qat_bn_ops.c 
               ${CMAKE_CURRENT_LIST_DIR}/he_qat_context.c)
add_library(he_qat SHARED ${HE_QAT_SRC})
target_include_directories(he_qat PUBLIC ${COMMON_INC_DIR})
target_include_directories(he_qat PUBLIC /opt/openssl/include)
target_link_directories(he_qat PUBLIC ${ICP_BUILDOUTPUT_PATH})
target_link_directories(he_qat PUBLIC /opt/openssl/lib)
target_link_libraries(he_qat PRIVATE cpa_sample_utils)
target_link_libraries(he_qat PUBLIC qat_s usdm_drv_s)
target_link_libraries(he_qat PUBLIC crypto ssl)
target_link_libraries(he_qat PUBLIC z pthread)

#add_definitions(-D_DESTINY_DEBUG_VERBOSE)
if(NOT HE_QAT_DEBUG_VERBOSE)
    remove_definitions(-D_DESTINY_DEBUG_VERBOSE) 
endif()

add_executable(test_context test_context.c)
target_link_libraries(test_context PUBLIC he_qat)
target_link_libraries(test_context PUBLIC cpa_sample_utils)
target_link_libraries(test_context PUBLIC z pthread ssl crypto qat_s usdm_drv_s)

set(IPPCP_DIR "/opt/ipp-crypto")
set(IPPCP_CMAKE_PREFIX_DIR "${IPPCP_DIR}/lib/cmake")
list(APPEND CMAKE_PREFIX_PATH "${IPPCP_CMAKE_PREFIX_DIR}")
find_package(ippcp REQUIRED)
#find_package(IPPCrypto REQUIRED MODULE)
#if (NOT IPPCRYPTO_FOUND)
#   message(FATAL_ERROR "No Intel IPP Cryptography library found on the system.")
#endif()
#if(NOT TARGET ippcp)
#    message(FATAL_ERROR "TARGET IPPCP::ippcp not found")
#else()
#    message(STATUS "TARGET IPPCP::ippcp found")
#endif()

set(HE_QAT_UTILS_INC_DIR "")
add_subdirectory(utils)

add_executable(test_bnModExpPerformOp test_bnModExpPerformOp.c)
target_link_libraries(test_bnModExpPerformOp PUBLIC he_qat)
target_link_libraries(test_bnModExpPerformOp PUBLIC cpa_sample_utils)
target_link_libraries(test_bnModExpPerformOp PUBLIC z pthread ssl crypto qat_s usdm_drv_s)

add_compile_options(-fpermissive)
add_executable(test_ippModExpPerformOp test_ippModExpPerformOp.cpp)
target_include_directories(test_ippModExpPerformOp PUBLIC ${COMMON_INC_DIR} ${HE_QAT_UTILS_INC_DIR})
target_link_libraries(test_ippModExpPerformOp PUBLIC he_qat)
target_link_libraries(test_ippModExpPerformOp PUBLIC he_qat_utils)
target_link_libraries(test_ippModExpPerformOp PUBLIC cpa_sample_utils)
target_link_libraries(test_ippModExpPerformOp PUBLIC ippcpmx crypto_mb)
target_link_libraries(test_ippModExpPerformOp PUBLIC z pthread ssl crypto qat_s usdm_drv_s)

add_compile_options(-fpermissive)
add_executable(test_bnConversion test_bnConversion.cpp)
target_include_directories(test_bnConversion PUBLIC ${COMMON_INC_DIR} ${HE_QAT_UTILS_INC_DIR})
target_link_libraries(test_bnConversion PUBLIC he_qat)
target_link_libraries(test_bnConversion PUBLIC he_qat_utils)
target_link_libraries(test_bnConversion PUBLIC cpa_sample_utils)
target_link_libraries(test_bnConversion PUBLIC ippcpmx crypto_mb)
target_link_libraries(test_bnConversion PUBLIC z pthread ssl crypto qat_s usdm_drv_s)
# Compile test case
#add_subdirectory(ln_mod_exp)

