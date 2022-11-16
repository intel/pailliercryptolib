# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

include(ExternalProject)
message(STATUS "Configuring cereal")
set(CEREAL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ext_cereal)
set(CEREAL_GIT_REPO_URL https://github.com/USCiLab/cereal.git)
set(CEREAL_GIT_LABEL ebef1e929807629befafbb2918ea1a08c7194554) # cereal - v1.3.2

ExternalProject_Add(
    ext_cereal
    PREFIX ${CEREAL_PREFIX}
    GIT_REPOSITORY ${CEREAL_GIT_REPO_URL}
    GIT_TAG ${CEREAL_GIT_LABEL}
    UPDATE_COMMAND ""
    EXCLUDE_FROM_ALL
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(ext_cereal SOURCE_DIR BINARY_DIR)
set(CEREAL_INC_DIR ${SOURCE_DIR}/include)
