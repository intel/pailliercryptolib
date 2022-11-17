# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# Add dependency to the target archive
function(ipcl_create_archive target dependency)
  # For proper export of IPCLConfig.cmake / IPCLTargets.cmake,
  # we avoid explicitly linking dependencies via target_link_libraries, since
  # this would add dependencies to the exported ipcl target.
  add_dependencies(${target} ${dependency})

  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_custom_command(TARGET ${target} POST_BUILD
                      COMMAND ar -x $<TARGET_FILE:${target}>
                      COMMAND ar -x $<TARGET_FILE:${dependency}>
                      COMMAND ar -qcs $<TARGET_FILE:${target}> *.o
                      COMMAND rm -f *.o
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      DEPENDS ${target} ${dependency}
      )
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_custom_command(TARGET ${target} POST_BUILD
                       COMMAND lib.exe /OUT:$<TARGET_FILE:${target}>
                        $<TARGET_FILE:${target}>
                        $<TARGET_FILE:${dependency}>
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        DEPENDS ${target} ${dependency}
  )
  else()
    message(WARNING "Unsupported compiler ${CMAKE_CXX_COMPILER_ID}")
  endif()
endfunction()


function(ipcl_detect_lscpu_flag flag verbose)
  # Detect IFMA by parsing lscpu
  set(LSCPU_FLAG ${flag})
  execute_process(COMMAND lscpu COMMAND grep ${LSCPU_FLAG} OUTPUT_VARIABLE LSCPU_FLAG)
  if("${LSCPU_FLAG}" STREQUAL "")
    if(verbose)
      message(STATUS "Support ${flag}: False")
    endif()
    set(IPCL_FOUND_${flag} FALSE PARENT_SCOPE)
  else()
    if(verbose)
      message(STATUS "Support ${flag}: True")
    endif()
    set(IPCL_FOUND_${flag} TRUE PARENT_SCOPE)
  endif()
endfunction()

function(ipcl_detect_qat)
  # Detect SPR based QAT
  message(STATUS "Detecting QAT...... ")
  set(IPCL_FOUND_QAT FALSE PARENT_SCOPE)

  if(DEFINED ENV{ICP_ROOT})
    # Validate environment variable ICP_ROOT
    set(tmp_ICP_ROOT $ENV{ICP_ROOT})
    get_filename_component(tmp_ICP_ROOT_fullpath "${tmp_ICP_ROOT}" REALPATH)
    if(EXISTS "${tmp_ICP_ROOT_fullpath}" AND
       EXISTS "${tmp_ICP_ROOT_fullpath}/build" AND
       EXISTS "${tmp_ICP_ROOT_fullpath}/quickassist")
      message(STATUS "Environment variable ICP_ROOT is defined as ${tmp_ICP_ROOT_fullpath}.")
      execute_process(COMMAND lspci -d 8086:4940 COMMAND wc -l OUTPUT_VARIABLE QAT_PHYSICAL OUTPUT_STRIP_TRAILING_WHITESPACE)
      if(${QAT_PHYSICAL} GREATER_EQUAL "1")
        message(STATUS "Detected ${QAT_PHYSICAL} physical QAT processes")
        execute_process(COMMAND lspci -d 8086:4941 COMMAND wc -l OUTPUT_VARIABLE QAT_VIRTUAL OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(${QAT_VIRTUAL} GREATER_EQUAL "1")
          message(STATUS "Detected ${QAT_VIRTUAL} virtual QAT processes")
          ipcl_check_qat_service_status()
          set(IPCL_FOUND_QAT TRUE PARENT_SCOPE)
        else()
          message(WARNING "NO virtual QAT processors - IPCL_ENABLE_QAT set to OFF")
        endif()
      else()
        message(WARNING "NO physical QAT processors - IPCL_ENABLE_QAT set to OFF")
      endif()
    else()
      message(WARNING "Environment variable ICP_ROOT is incorrect - IPCL_ENABLE_QAT set to OFF")
    endif()
  else()
  	message(WARNING "Environment variable ICP_ROOT must be defined - IPCL_ENABLE_QAT set to OFF")
  endif()
endfunction()


function(ipcl_check_qat_service_status)
  # Detect qat_service service status
  execute_process(COMMAND systemctl status qat_service.service COMMAND grep "Active: active" COMMAND wc -l OUTPUT_VARIABLE QAT_SERVICE_STATUS OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(${QAT_SERVICE_STATUS} EQUAL "1")
    message(STATUS "qat_service is ACTIVE")
  else()
    message(WARNING
      " qat_service is NOT ACTIVE!\n"
      " Since QAT is detected, compilation will continue however the"
      " qat_service need to be active to use the library.\n"
      " To start the service, issue the following command --"
      " \$ sudo systemctl start qat_service.service"
    )
  endif()
endfunction()

function(ipcl_define_icp_variables OutVariable)
  set(ICP_ROOT             $ENV{ICP_ROOT})
  set(ICP_BUILDOUTPUT_PATH ${ICP_ROOT}/build)
  set(ICP_BUILDSYSTEM_PATH ${ICP_ROOT}/quickassist/build_system)
  set(ICP_API_DIR          ${ICP_ROOT}/quickassist)
  set(ICP_LAC_DIR          ${ICP_ROOT}/quickassist/lookaside/access_layer)
  set(ICP_OSAL_DIR         ${ICP_ROOT}/quickassist/utilities/oasl)
  set(ICP_ADF_DIR          ${ICP_ROOT}/quickassist/lookaside/access_layer/src/qat_direct)
  set(CMN_ROOT             ${ICP_ROOT}/quickassist/utilities/libusdm_drv)

  set(${OutVariable} ${ICP_API_DIR}/include
                  ${ICP_LAC_DIR}/include
                  ${ICP_ADF_DIR}/include
                  ${CMN_ROOT}
                  ${ICP_API_DIR}/include/dc
                  ${ICP_API_DIR}/include/lac
                  PARENT_SCOPE)
endfunction()

function(ipcl_get_core_thread_count cores threads verbose)
  include(ProcessorCount)

  # Get number threads
  ProcessorCount(N)
  set(${threads} ${N} PARENT_SCOPE)

  # parse smt active
  execute_process(COMMAND cat /sys/devices/system/cpu/smt/active OUTPUT_VARIABLE IS_HYPERTHREADING)
  if("${IS_HYPERTHREADING}" STREQUAL "1")
    math(EXPR n_cores "${N} / 2" )
    if(verbose)
      message(STATUS "Hyperthreading is ON")
      message(STATUS "# of physical cores: ${n_cores}")
    endif()
    set(${cores} ${n_cores} PARENT_SCOPE)
  else()
    if(verbose)
      message(STATUS "Hyperthreading is OFF")
      message(STATUS "# of physical cores: ${N}")
    endif()
    set(${cores} ${N} PARENT_SCOPE)
  endif()
endfunction()
