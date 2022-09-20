/**
 * @file he_qat_bn_ops.h
 *
 * @details 
 * 	In this file, functions for Big Number operations accelerated by the 
 * 	QuickAssist (QAT) co-processor are specified.
 *
 * @note
 * 	Unless otherwise specified, Big numbers are represented by octet strings 
 * 	and stored in memory as pointers of type unsigned char*. On the QAT API the 
 * 	octect string is copied into a data structure of type CpaFlatBuffer. The octet 
 * 	strings representing Big Numbers are encoded with compliance to PKCA#1 v2.1, 
 * 	section 4, which is consistent with ASN.1 syntax. 
 *
 *	The largest number supported here has 4096 bits, i.e. numbers from 0 to 
 *	2^(4096)-1. The most significant bit is located at index n-1. If the number is 
 *	N, then the bit length is defined by n = floor(log2(N))+1. The memory buffer b to 
 *	hold such number N needs to have at least M = ceiling(n/8) bytes allocated. In 
 *	general, it will be larger and a power of 2, e.g. total bytes allocated is T=128 
 *	for numbers having up to n=1024 bits, total bytes allocated is T=256 for numbers 
 *	having up to n=2048 bits, and so forth. Finally, the big number N is stored in 
 *	`big endian` format, i.e. the least significant byte (LSB) is located at index [T-1], 
 *	whereas the most significant byte is stored at [T-M].
 *
 * 	The API client is responsible for allocation and release of their memory spaces of 
 * 	the function arguments. Allocated memory spaces must be contiguous. Once a function 
 * 	is called, the ownership of the memory spaces is transfered to the function until 
 * 	their completion such that concurrent usage by the client during excution may result 
 * 	in undefined behavior.
 */

// New compilers
#pragma once

// Legacy compilers
#ifndef _HE_QAT_BN_OPS_H_
#define _HE_QAT_BN_OPS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/bn.h>
#include "he_qat_types.h"

/// @brief Modular exponentiation using BIGNUM data structure.
/// Perform big number modular exponentiation operation accelerated with QAT for input data 
/// using OpenSSL BIGNUM data structure.
/// 
/// @details
/// Create private buffer for code section. Create QAT contiguous memory space.
/// Copy data and package into a request and call producer function to submit
/// request into qat buffer.
///
/// @param[out] r Remainder number of the modular exponentiation operation.
/// @param[in] b Base number of the modular exponentiation operation.
/// @param[in] e Exponent number of the modular exponentiation operation.
/// @param[in] m Modulus number of the modular exponentiation operation.
/// @param[in] nbits Number of bits (bit precision) of input/output big numbers.
HE_QAT_STATUS HE_QAT_BIGNUMModExp(BIGNUM* r, BIGNUM* b, BIGNUM* e, BIGNUM* m, int nbits);

/// @brief
/// Generic big number modular exponentiation for input data in primitive type
/// format (unsigned char *).
/// 
/// @details
/// Create private buffer for code section. Create QAT contiguous memory space.
/// Copy data and package into a request and call producer function to submit
/// request into qat buffer.
/// 
/// @param[out] r Remainder number of the modular exponentiation operation.
/// @param[in] b Base number of the modular exponentiation operation.
/// @param[in] e Exponent number of the modular exponentiation operation.
/// @param[in] m Modulus number of the modular exponentiation operation.
/// @param[in] nbits Number of bits (bit precision) of input/output big numbers.
HE_QAT_STATUS HE_QAT_bnModExp(unsigned char* r, unsigned char* b,
                              unsigned char* e, unsigned char* m, int nbits);

/// @brief 
/// It waits for number of requests sent by HE_QAT_bnModExp or bnModExpPerformOp 
/// to complete.
/// 
/// @details
/// This function is blocking and works as barrier. The purpose of this function 
/// is to wait for all outstanding requests to complete processing. It will also 
/// release all temporary memory allocated to support the requests.
/// Monitor outstanding request to be complete and then deallocate buffer
/// holding outstanding request.
///
/// @param[in] num_requests Number of requests to wait to complete processing.
void getBnModExpRequest(unsigned int num_requests);

/** 
 *
 * Interfaces for the Multithreading Support 
 *
 **/

/// @brief
/// Generic big number modular exponentiation for input data in primitive type
/// format (unsigned char *).
/// 
/// @details
/// Create private buffer for code section. Create QAT contiguous memory space.
/// Copy data and package into a request and call producer function to submit
/// request into qat buffer.
/// 
/// @param[in] _buffer_id Buffer ID of the reserved buffer for the caller's thread.
/// @param[out] r Remainder number of the modular exponentiation operation.
/// @param[in] b Base number of the modular exponentiation operation.
/// @param[in] e Exponent number of the modular exponentiation operation.
/// @param[in] m Modulus number of the modular exponentiation operation.
/// @param[in] nbits Number of bits (bit precision) of input/output big numbers.
HE_QAT_STATUS HE_QAT_bnModExp_MT(unsigned int _buffer_id, unsigned char* r,
                                 unsigned char* b, unsigned char* e, unsigned char* m, int nbits);

/// @brief Reserve/acquire buffer for multithreading support.
/// @details Try to acquire an available buffer to store outstanding work requests sent by caller.  
///          If none is available, it blocks further processing and waits until another caller's concurrent 
///          thread releases one. This function must be called before
///          calling HE_QAT_bnModExp_MT(.).
/// @param[out] _buffer_id Pointer to memory space allocated by caller to hold the buffer ID of the buffer used to store caller's outstanding requests.
HE_QAT_STATUS acquire_bnModExp_buffer(unsigned int* _buffer_id);

/// @brief Wait for request processing to complete and release previously acquired buffer. 
/// @details Caution: It assumes acquire_bnModExp_buffer(&_buffer_id) to be called first
/// to secure and be assigned an outstanding buffer for the target thread. 
/// Equivalent to getBnModExpRequests() for the multithreading support interfaces.
/// param[in] _buffer_id Buffer ID of the buffer to be released/unlock for reuse by the next concurrent thread.
/// param[in] _batch_size Total number of requests to wait for completion before releasing the buffer.
void release_bnModExp_buffer(unsigned int _buffer_id, unsigned int _batch_size);

#ifdef __cplusplus
}  // extern "C" {
#endif

#endif
