
#add_definitions(-D_DESTINY_DEBUG_VERBOSE)
if (NOT HE_QAT_DEBUG_VERBOSE)
    remove_definitions(-D_DESTINY_DEBUG_VERBOSE) 
endif()

add_executable(test_context test_context.c)
target_include_directories(test_context PUBLIC ${COMMON_INC_DIR} ${HE_QAT_MISC_INC_DIR})
target_link_libraries(test_context PUBLIC he_qat)
#target_link_libraries(test_context PUBLIC cpa_sample_utils)
target_link_libraries(test_context PUBLIC z pthread ssl crypto qat_s usdm_drv_s)

add_executable(test_bnModExpPerformOp test_bnModExpPerformOp.c)
target_include_directories(test_bnModExpPerformOp PUBLIC ${COMMON_INC_DIR}) # ${HE_QAT_UTILS_INC_DIR})
target_link_libraries(test_bnModExpPerformOp PUBLIC he_qat)
#target_link_libraries(test_bnModExpPerformOp PUBLIC cpa_sample_utils)
target_link_libraries(test_bnModExpPerformOp PUBLIC z pthread ssl crypto qat_s usdm_drv_s)

add_compile_options(-fpermissive)
add_executable(test_bnConversion test_bnConversion.cpp)
target_include_directories(test_bnConversion PUBLIC  ${COMMON_INC_DIR} ${HE_QAT_MISC_INC_DIR})
target_link_libraries(test_bnConversion PUBLIC he_qat)
target_link_libraries(test_bnConversion PUBLIC he_qat_misc)
target_link_libraries(test_bnConversion PUBLIC z pthread ssl crypto qat_s usdm_drv_s)
