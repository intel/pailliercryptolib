// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
/// @file heqat/common/utils.h

#pragma once

#ifndef HE_QAT_UTILS_H_
#define HE_QAT_UTILS_H_

#ifdef __cplusplus
#define HE_QAT_RESTRICT __restrict__
#else
#define HE_QAT_RESTRICT restrict
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "heqat/common/types.h"

#include <openssl/bn.h>
#include <errno.h>

#include <qae_mem.h>

#ifndef BYTE_ALIGNMENT_8
#define BYTE_ALIGNMENT_8 (8)
#endif
#ifndef BYTE_ALIGNMENT_64
#define BYTE_ALIGNMENT_64 (64)
#endif

// 5 seconds
#ifndef TIMEOUT_MS
#define TIMEOUT_MS 5000 
#endif

// Printing Functions
#ifdef HE_QAT_DEBUG
#define HE_QAT_PRINT_DBG(args...)                                              \
    do                                                                         \
    {                                                                          \
        printf("%s(): ", __func__);                                            \
        printf(args);                                                          \
        fflush(stdout);                                                        \
    } while (0)
#else
#define HE_QAT_PRINT_DBG(args...) { } 
#endif

#ifndef HE_QAT_PRINT
#define HE_QAT_PRINT(args...)                                                  \
    do                                                                         \
    {                                                                          \
        printf(args);                                                          \
    } while (0)
#endif

#ifndef HE_QAT_PRINT_ERR
#define HE_QAT_PRINT_ERR(args...)                                              \
    do                                                                         \
    {                                                                          \
        printf("%s(): ", __func__);                                            \
        printf(args);                                                          \
    } while (0)
#endif

// Use semaphores to signal completion of events
#define COMPLETION_STRUCT completion_struct
#define COMPLETION_INIT(s) sem_init(&((s)->semaphore), 0, 0);
#define COMPLETION_WAIT(s, timeout) (sem_wait(&((s)->semaphore)) == 0)
#define COMPLETE(s) sem_post(&((s)->semaphore))
#define COMPLETION_DESTROY(s) sem_destroy(&((s)->semaphore))

/// @brief
///      This function and associated macro allocates the memory for the given
///      size for the given alignment and stores the address of the memory
///      allocated in the pointer. Memory allocated by this function is
///      guaranteed to be physically contiguous.
/// 
/// @param[out] ppMemAddr    address of pointer where address will be stored
/// @param[in] sizeBytes     the size of the memory to be allocated
/// @param[in] alignment     the alignment of the memory to be allocated (non-zero)
/// @param[in] node          the allocate memory that is local to cpu(node)
/// 
/// @retval HE_QAT_STATUS_FAIL      Failed to allocate memory.
/// @retval HE_QAT_STATUS_SUCCESS   Memory successfully allocated.
static __inline HE_QAT_STATUS HE_QAT_memAllocContig(void **ppMemAddr,
                                           Cpa32U sizeBytes,
                                           Cpa32U alignment, 
					                       Cpa32U node)
{
    *ppMemAddr = qaeMemAllocNUMA(sizeBytes, node, alignment);
    if (NULL == *ppMemAddr) {
        return HE_QAT_STATUS_FAIL;
    }
    return HE_QAT_STATUS_SUCCESS;
}
#define HE_QAT_MEM_ALLOC_CONTIG(ppMemAddr, sizeBytes, alignment) \
    HE_QAT_memAllocContig((void *)(ppMemAddr), (sizeBytes), (alignment), 0)


/// @brief
///      This function and associated macro frees the memory at the given address
///      and resets the pointer to NULL. The memory must have been allocated by
///      the function Mem_Alloc_Contig().
/// 
/// @param[out] ppMemAddr    address of pointer where mem address is stored.
///                          If pointer is NULL, the function will exit silently
static __inline void HE_QAT_memFreeContig(void **ppMemAddr)
{
    if (NULL != *ppMemAddr) {
        qaeMemFreeNUMA(ppMemAddr);
        *ppMemAddr = NULL;
    }
}
#define HE_QAT_MEM_FREE_CONTIG(pMemAddr) \
    HE_QAT_memFreeContig((void *)&pMemAddr)


/// @brief Sleep for time unit.
/// @param[in] time Unsigned integer representing amount of time.
/// @param[in] unit Time unit of the amount of time passed in the first parameter. Unit values can be HE_QAT_NANOSEC (nano seconds), HE_QAT_MICROSEC (micro seconds), HE_QAT_MILLISEC (milli seconds), or HE_QAT_SEC (seconds).
static __inline HE_QAT_STATUS HE_QAT_sleep(unsigned int time, HE_QAT_TIME_UNIT unit)
{
    int ret = 0;
    struct timespec resTime, remTime;

    resTime.tv_sec = time / unit;
    resTime.tv_nsec = (time % unit) * (HE_QAT_NANOSEC/unit);

    do {
      ret = nanosleep(&resTime, &remTime);
      resTime = remTime;
    } while ((0 != ret) && (EINTR == errno));

    if (0 != ret) {
      HE_QAT_PRINT_ERR("nano sleep failed with code %d\n", ret);
      return HE_QAT_STATUS_FAIL;
    } else {
      return HE_QAT_STATUS_SUCCESS;
    }
}
#define HE_QAT_SLEEP(time, timeUnit) \
    HE_QAT_sleep((time), (timeUnit))

/// @brief
///      This function returns the physical address for a given virtual address.
///      In case of error 0 is returned.
/// @param[in] virtAddr     Virtual address
/// @retval CpaPhysicalAddr Physical address or 0 in case of error
static __inline CpaPhysicalAddr HE_QAT_virtToPhys(void *virtAddr)
{
    return (CpaPhysicalAddr)qaeVirtToPhysNUMA(virtAddr);
}

BIGNUM* generateTestBNData(int nbits);

unsigned char* paddingZeros(BIGNUM* bn, int nbits);

void showHexBN(BIGNUM* bn, int nbits);

void showHexBin(unsigned char* bin, int len);

#ifdef __cplusplus
}  // extern "C" {
#endif

#endif  // HE_QAT_UTILS_H_
