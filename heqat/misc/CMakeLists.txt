# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# Add MISC source code
set(HE_QAT_MISC_SRC ${CMAKE_CURRENT_LIST_DIR}/misc.cpp 
                    ${CMAKE_CURRENT_LIST_DIR}/utils.cpp 
                    ${CMAKE_CURRENT_LIST_DIR}/bignum.cpp)
list(APPEND HE_QAT_SRC ${HE_QAT_MISC_SRC})
set(HE_QAT_SRC ${HE_QAT_SRC} PARENT_SCOPE)
