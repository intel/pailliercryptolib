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
