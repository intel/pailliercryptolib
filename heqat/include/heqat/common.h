/// @file heqat/common.h

#include "cpa_sample_utils.h"
#include "heqat/common/consts.h"
#include "heqat/common/types.h"
#include "heqat/common/utils.h"

#ifdef __cplusplus
#ifdef HE_QAT_MISC
#include "heqat/misc.h"
#endif
#endif

#ifdef HE_QAT_DEBUG
#define HE_QAT_PRINT_DBG(args...)                                              \
    do                                                                         \
    {                                                                          \
        {                                                                      \
            printf("%s(): ", __func__);                                        \
            printf(args);                                                      \
            fflush(stdout);                                                    \
        }                                                                      \
    } while (0)
#else
#define HE_QAT_PRINT_DBG(args...) { } 
#endif