project(he_qat_example LANGUAGES C CXX)

cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 11)
set(HE_QAT_HINT_DIR ${CMAKE_PREFIX_PATH})
message(STATUS "CMAKE_PREFIX_PATH ${HE_QAT_HINT_DIR}")

# Example using source
find_package(HE_QAT 1.3.0 
	     HINTS ${HE_QAT_HINT_DIR} 
	     REQUIRED)
if(NOT TARGET HE_QAT::he_qat)
  message(FATAL_ERROR "TARGET HE_QAT::he_qat not found")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  add_definitions(-DHE_QAT_DEBUG)
endif()

find_package(OpenSSL REQUIRED)

include(../icp/CMakeLists.txt)

add_definitions(-fpermissive)
add_executable(example example.cpp)
target_include_directories(example PRIVATE ${ICP_INC_DIR})
target_link_libraries(example PRIVATE HE_QAT::he_qat)
target_link_libraries(example PRIVATE qat_s)
target_link_libraries(example PRIVATE OpenSSL::SSL)
