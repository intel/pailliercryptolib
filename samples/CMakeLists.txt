
if(HE_QAT_DEBUG)
    add_definitions(-D_DESTINY_DEBUG_VERBOSE) 
endif()

# -------------------------------------------------------------------------------------------

add_executable(test_context test_context.c)

target_include_directories(test_context PUBLIC ${COMMON_INC_DIR})
#target_include_directories(test_context PUBLIC ${HE_QAT_INC_DIR})
target_include_directories(test_context PUBLIC ${ICP_INC_DIR})

target_link_libraries(test_context PUBLIC he_qat)

install(TARGETS test_context RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# -------------------------------------------------------------------------------------------

add_executable(test_BIGNUMModExp test_BIGNUMModExp.c)

target_include_directories(test_BIGNUMModExp PUBLIC ${COMMON_INC_DIR}) # ${HE_QAT_UTILS_INC_DIR})
#target_include_directories(test_BIGNUMModExp PUBLIC ${HE_QAT_INC_DIR})
target_include_directories(test_BIGNUMModExp PUBLIC ${ICP_INC_DIR})

target_link_libraries(test_BIGNUMModExp PUBLIC he_qat)
target_link_libraries(test_BIGNUMModExp PUBLIC OpenSSL::SSL) # OpenSSL::Crypto)
#target_link_libraries(test_BIGNUMModExp PUBLIC IPPCP::ippcp)

install(TARGETS test_BIGNUMModExp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# -------------------------------------------------------------------------------------------

if(HE_QAT_MISC)
  add_compile_options(-fpermissive)
  
  # Sample showing how to convert from/to BigNumber to/from CpaFlatBuffer
  add_executable(test_bnConversion test_bnConversion.cpp)
  
  target_include_directories(test_bnConversion PUBLIC ${COMMON_INC_DIR})
  target_include_directories(test_bnConversion PUBLIC ${ICP_INC_DIR})
  target_include_directories(test_bnConversion PUBLIC ${HE_QAT_MISC_INC_DIR})
  
  target_link_libraries(test_bnConversion PUBLIC he_qat)
  target_link_libraries(test_bnConversion PUBLIC he_qat_misc)
  target_link_libraries(test_bnConversion PUBLIC IPPCP::ippcp) # IPPCP::crypto_mb)
  target_link_libraries(test_bnConversion PUBLIC OpenSSL::SSL) # OpenSSL::Crypto)
  
  install(TARGETS test_bnConversion RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
 
  # Sample demonstrating how to use bnModExp
  add_executable(test_bnModExp test_bnModExp.cpp)
  
  target_include_directories(test_bnModExp PUBLIC ${COMMON_INC_DIR})
  target_include_directories(test_bnModExp PUBLIC ${ICP_INC_DIR})
  target_include_directories(test_bnModExp PUBLIC ${HE_QAT_MISC_INC_DIR})
  
  target_link_libraries(test_bnModExp PUBLIC he_qat)
  target_link_libraries(test_bnModExp PUBLIC he_qat_misc)
  target_link_libraries(test_bnModExp PUBLIC IPPCP::ippcp) # IPPCP::crypto_mb)
  target_link_libraries(test_bnModExp PUBLIC OpenSSL::SSL) # OpenSSL::Crypto)
  
  install(TARGETS test_bnModExp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
 
  if(OpenMP_CXX_FOUND) 
    # Sample demonstrating how to use multithread-supported interface bnModExp_MT 
    add_executable(test_bnModExp_MT test_bnModExp_MT.cpp)
    
    target_include_directories(test_bnModExp_MT PUBLIC ${COMMON_INC_DIR})
    target_include_directories(test_bnModExp_MT PUBLIC ${ICP_INC_DIR})
    target_include_directories(test_bnModExp_MT PUBLIC ${HE_QAT_MISC_INC_DIR})
    
    target_link_libraries(test_bnModExp_MT PUBLIC he_qat)
    target_link_libraries(test_bnModExp_MT PUBLIC he_qat_misc)
    target_link_libraries(test_bnModExp_MT PUBLIC OpenMP::OpenMP_CXX)
    target_link_libraries(test_bnModExp_MT PUBLIC IPPCP::ippcp) # IPPCP::crypto_mb)
    target_link_libraries(test_bnModExp_MT PUBLIC OpenSSL::SSL) # OpenSSL::Crypto)
    
    install(TARGETS test_bnModExp_MT RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()
endif()
