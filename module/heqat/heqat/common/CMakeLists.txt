# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# Source files
set(COMMON_SRC ${CMAKE_CURRENT_LIST_DIR}/utils.c)
list(APPEND HE_QAT_SRC ${COMMON_SRC})
set(HE_QAT_SRC ${HE_QAT_SRC} PARENT_SCOPE)

