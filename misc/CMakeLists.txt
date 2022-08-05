
#Include directories
#list(APPEND HE_QAT_MISC_INC_DIR
set(HE_QAT_MISC_INC_DIR ${COMMON_INC_DIR}	
	                ${HE_QAT_INC_DIR} 
			${CMAKE_CURRENT_LIST_DIR} 
			#			${IPPCP_DIR}/include
) 

message(STATUS "HE_QAT_MISC_INC_DIR: ${HE_QAT_MISC_INC_DIR}")

#Source files
#list(APPEND HE_QAT_MISC_SRC ${CMAKE_CURRENT_LIST_DIR} / utils.cpp
set(HE_QAT_MISC_SRC ${CMAKE_CURRENT_LIST_DIR}/he_qat_misc.cpp 
	            ${CMAKE_CURRENT_LIST_DIR}/utils.cpp 
		    ${CMAKE_CURRENT_LIST_DIR}/bignum.cpp)

add_library(he_qat_misc ${HE_QAT_MISC_SRC}) 
target_include_directories(he_qat_misc PUBLIC $<BUILD_INTERFACE:${HE_QAT_MISC_INC_DIR}>        #Public headers 
	                               PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> #Public headers
				       PRIVATE ${HE_QAT_MISC_INC_DIR}                          #Private headers 
				       PRIVATE ${ICP_INC_DIR}                          #Private headers 
)
#target_link_directories(he_qat_misc PUBLIC ${IPPCP_DIR}/lib/intel64)
#target_link_libraries(he_qat_misc PRIVATE ippcpmx crypto_mb)
target_link_libraries(he_qat_misc PRIVATE IPPCP::ippcp IPPCP::crypto_mb)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/
	DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/
	FILES_MATCHING 
	PATTERN "*.hpp"
	PATTERN "*.h")
install(TARGETS he_qat_misc DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(HE_QAT_MISC_INC_DIR ${HE_QAT_MISC_INC_DIR} PARENT_SCOPE)
