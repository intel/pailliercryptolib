cmake_minimum_required(VERSION 3.13)

    project(QATModExp LANGUAGES C CXX)

        set(CMAKE_C_STANDARD 99) set(CMAKE_C_STANDARD_REQUIRED ON) set(
            CMAKE_CXX_STANDARD 11) set(CMAKE_CXX_STANDARD_REQUIRED ON)

            include(CheckCCompilerFlag) include(CheckCXXCompilerFlag) include(
                CMakePackageConfigHelpers)

                set(HE_QAT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}) set(
                    HE_QAT_SRC_DIR ${HE_QAT_ROOT_DIR} /
                    he_qat) set(HE_QAT_INC_DIR ${HE_QAT_ROOT_DIR} / he_qat /
                                include)

#OpenSSL installation
                    set(OPENSSL_INC_DIR / opt / openssl /
                        include) set(OPENSSL_LIB_DIR / opt / openssl / lib)

                        set(COMMON_INC_DIR ${CMAKE_CURRENT_LIST_DIR} / include)

                            set(CMAKE_INSTALL_PREFIX "install")

#QAT lib installation
                                set(ICP_ROOT $ENV{ICP_ROOT}) set(
                                    ICP_BUILDOUTPUT_PATH ${ICP_ROOT} /
                                    build) set(ICP_BUILDSYSTEM_PATH ${
                                                   ICP_ROOT} /
                                               quickassist /
                                               build_system) set(ICP_API_DIR ${
                                                                     ICP_ROOT} /
                                                                 quickassist)
                                    set(ICP_LAC_DIR ${ICP_ROOT} / quickassist /
                                        lookaside /
                                        access_layer) set(ICP_OSAL_DIR ${
                                                              ICP_ROOT} /
                                                          quickassist /
                                                          utilities / oasl)
                                        set(ICP_ADF_DIR ${ICP_ROOT} /
                                            quickassist / lookaside /
                                            access_layer / src /
                                            qat_direct) set(CMN_ROOT ${
                                                                ICP_ROOT} /
                                                            quickassist /
                                                            utilities /
                                                            libusdm_drv)

                                            list(APPEND COMMON_INC_DIR ${
                                                     ICP_API_DIR} /
                                                 include ${ICP_LAC_DIR} /
                                                 include ${ICP_ADF_DIR} /
                                                 include ${CMN_ROOT})

#Macros for the test case
                                                add_definitions(-DDO_CRYPTO) add_definitions(
                                                    -DUSER_SPACE) add_compile_options(-fPIC)

#Common utility functions
                                                    add_subdirectory(common)

#IPP Crypto installation
                                                        set(IPPCP_DIR
                                                            "/opt/ipp-crypto")
                                                            set(IPPCP_CMAKE_PREFIX_DIR
                                                                "${IPPCP_DIR}/"
                                                                "lib/cmake")
                                                                list(
                                                                    APPEND
                                                                        CMAKE_PREFIX_PATH
                                                                    "${IPPCP_"
                                                                    "CMAKE_"
                                                                    "PREFIX_"
                                                                    "DIR}")
                                                                    find_package(
                                                                        ippcp
                                                                            REQUIRED)

#Helper functions
#set(HE_QAT_MISC_INC_DIR ${COMMON_INC_DIR} ${CMAKE_CURRENT_LIST_DIR})
#set(HE_QAT_MISC_INC_DIR ${COMMON_INC_DIR})
#set(HE_QAT_MISC_SRC ${HE_QAT_SRC_DIR} / he_qat_misc.cpp)
                                                                        add_subdirectory(
                                                                            misc)

#Library
                                                                            add_subdirectory(
                                                                                he_qat)

#Validation test examples
                                                                                add_subdirectory(
                                                                                    examples)
