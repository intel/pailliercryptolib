
#Include directories
#list(APPEND HE_QAT_MISC_INC_DIR
set(HE_QAT_MISC_INC_DIR ${COMMON_INC_DIR}	
	                ${HE_QAT_INC_DIR} 
			${CMAKE_CURRENT_LIST_DIR} 
			#		${IPPCP_DIR}/include
) 

message(STATUS "HE_QAT_MISC_INC_DIR: ${HE_QAT_MISC_INC_DIR}")

#Source files
#list(APPEND HE_QAT_MISC_SRC ${CMAKE_CURRENT_LIST_DIR} / utils.cpp
set(HE_QAT_MISC_SRC 
        	${CMAKE_CURRENT_LIST_DIR}/he_qat_misc.cpp 
	            ${CMAKE_CURRENT_LIST_DIR}/utils.cpp 
		    ${CMAKE_CURRENT_LIST_DIR}/bignum.cpp)

if(HE_QAT_SHARED)
	add_library(he_qat_misc SHARED ${HE_QAT_MISC_SRC}) 
else()
	add_library(he_qat_misc STATIC ${HE_QAT_MISC_SRC}) 
endif()

message(STATUS "IPPCP Headers Directory ${IPPCP_DIR}/include")
target_include_directories(he_qat_misc #PUBLIC $<BUILD_INTERFACE:${HE_QAT_MISC_INC_DIR}>        #Public headers 
	#PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> #Public headers
	PRIVATE ${IPPCP_DIR}/../../../include
				       PRIVATE ${HE_QAT_MISC_INC_DIR}                          #Private headers 
				       PRIVATE ${ICP_INC_DIR}                          #Private headers 

)
#target_link_directories(he_qat_misc PUBLIC ${IPPCP_DIR}/lib/intel64)
#target_link_libraries(he_qat_misc PRIVATE ippcpmx crypto_mb)

if(HE_QAT_SHARED)
  target_link_libraries(he_qat_misc PRIVATE IPPCP::ippcp IPPCP::crypto_mb)
else()
  heqat_create_archive(he_qat_misc IPPCP::ippcp)
  heqat_create_archive(he_qat_misc IPPCP::crypto_mb)
endif()

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/
	DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/
	FILES_MATCHING 
	PATTERN "*.hpp"
	PATTERN "*.h")
#install(TARGETS he_qat_misc DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS he_qat_misc EXPORT he_qatTargets DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(HE_QAT_MISC_INC_DIR ${HE_QAT_MISC_INC_DIR} PARENT_SCOPE)
