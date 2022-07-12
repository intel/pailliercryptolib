# Copyright (C) 2021 Intel Corporation


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was HE_QATConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

include(${CMAKE_CURRENT_LIST_DIR}/he_qatTargets.cmake)

if(TARGET HE_QAT::he_qat)
  set(HE_QAT_FOUND TRUE)
  message(STATUS "Intel Homomorphic Encryption Acceleration Library for QAT found")
else()
  message(STATUS "Intel Homomorphic Encryption Acceleraiton Library for QAT not found")
endif()

set(HE_QAT_VERSION "@HE_QAT_VERSION")
set(HE_QAT_VERSION_MAJOR "@HE_QAT_VERSION_MAJOR")
set(HE_QAT_VERSION_MINOR "@HE_QAT_VERSION")
set(HE_QAT_VERSION_PATCH "@HE_QAT_VERSION")

set(HE_QAT_DEBUG "@HE_QAT_DEBUG")
