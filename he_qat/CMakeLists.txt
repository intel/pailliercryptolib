
set(HE_QAT_SRC ${HE_QAT_SRC_DIR} / he_qat_bn_ops.c ${HE_QAT_SRC_DIR} /
    he_qat_context.c)

    add_library(he_qat SHARED ${HE_QAT_SRC})

        target_include_directories(
            he_qat PUBLIC $<
                BUILD_INTERFACE : ${COMMON_INC_DIR}> #Public headers PUBLIC
                $<BUILD_INTERFACE : ${HE_QAT_INC_DIR}> #Public headers PUBLIC $<
                    BUILD_INTERFACE : ${OPENSSL_INC_DIR}> #Public headers PUBLIC
                    $<INSTALL_INTERFACE : ${CMAKE_INSTALL_INCLUDEDIR}> #Public headers
                        PRIVATE $ { COMMON_INC_DIR } #Private headers PRIVATE
                            $ { OPENSSL_INC_DIR } #Private headers)
#target_include_directories(he_qat PUBLIC ${HE_QAT_UTILS_INC_DIR})

            target_link_directories(he_qat PUBLIC ${ICP_BUILDOUTPUT_PATH})
                target_link_directories(he_qat PUBLIC ${OPENSSL_LIB_DIR})
                    target_link_libraries(he_qat PRIVATE cpa_sample_utils)
#target_link_libraries(he_qat PRIVATE he_qat_utils)
                        target_link_libraries(he_qat PUBLIC qat_s usdm_drv_s)
                            target_link_libraries(he_qat PUBLIC crypto ssl)
                                target_link_libraries(he_qat PUBLIC z pthread)
