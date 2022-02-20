
set(LNMODEXP_SRC cpa_mod_exp_sample.c)

list(APPEND COMMON_INC_DIR ${CMAKE_CURRENT_LIST_DIR})

add_library(ln_mod_exp SHARED ${LNMODEXP_SRC})
target_include_directories(ln_mod_exp PUBLIC ${COMMON_INC_DIR})
target_link_directories(ln_mod_exp PUBLIC ${ICP_BUILDOUTPUT_PATH})
target_link_libraries(ln_mod_exp PRIVATE cpa_sample_utils)
target_link_libraries(ln_mod_exp PUBLIC z pthread crypto qat_s usdm_drv_s)

add_executable(test_ln_mod_exp cpa_mod_exp_sample_user.c)
target_include_directories(test_ln_mod_exp PUBLIC ${COMMON_INC_DIR})
target_link_directories(test_ln_mod_exp PUBLIC ${ICP_BUILDOUTPUT_PATH})
target_include_directories(test_ln_mod_exp PUBLIC /opt/openssl/include)
target_link_directories(test_ln_mod_exp PUBLIC /opt/openssl/lib)
target_link_libraries(test_ln_mod_exp PRIVATE cpa_sample_utils)
target_link_libraries(test_ln_mod_exp PRIVATE ln_mod_exp)
target_link_libraries(test_ln_mod_exp PUBLIC z pthread crypto ssl crypto qat_s usdm_drv_s)

add_executable(qat_modexp_validation qat_modexp_validation.c)
target_include_directories(qat_modexp_validation PUBLIC ${COMMON_INC_DIR})
target_link_directories(qat_modexp_validation PUBLIC ${ICP_BUILDOUTPUT_PATH})
target_include_directories(qat_modexp_validation PUBLIC /opt/openssl/include)
target_link_directories(qat_modexp_validation PUBLIC /opt/openssl/lib)
target_link_libraries(qat_modexp_validation PRIVATE cpa_sample_utils)
target_link_libraries(qat_modexp_validation PRIVATE ln_mod_exp)
target_link_libraries(qat_modexp_validation PUBLIC z pthread crypto ssl crypto qat_s usdm_drv_s)

install(TARGETS ln_mod_exp DESTINATION "lib")
install(TARGETS test_ln_mod_exp DESTINATION "bin")
install(TARGETS qat_modexp_validation DESTINATION "bin")

