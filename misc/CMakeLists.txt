
# Include directories
list(APPEND HE_QAT_MISC_INC_DIR  
	${CMAKE_CURRENT_LIST_DIR}
	${IPPCP_DIR}/include
)
message(STATUS "HE_QAT_MISC_INC_DIR: ${HE_QAT_MISC_INC_DIR}")

# Source files
list(APPEND HE_QAT_MISC_SRC ${CMAKE_CURRENT_LIST_DIR}/utils.cpp
                             ${CMAKE_CURRENT_LIST_DIR}/bignum.cpp
)

add_library(he_qat_misc ${HE_QAT_MISC_SRC})
target_include_directories(he_qat_misc
	PUBLIC  $<BUILD_INTERFACE:${HE_QAT_MISC_INC_DIR}>               # Public headers
    PUBLIC  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>   # Public headers
)
target_link_directories(he_qat_misc PUBLIC ${IPPCP_DIR}/lib/intel64)
target_link_libraries(he_qat_misc PUBLIC ippcpmx crypto_mb)

set(HE_QAT_MISC_INC_DIR ${HE_QAT_MISC_INC_DIR} PARENT_SCOPE)

