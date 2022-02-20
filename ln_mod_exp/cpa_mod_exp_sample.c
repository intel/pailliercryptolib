/***************************************************************************
 *
 *   BSD LICENSE
 * 
 *   Copyright(c) 2007-2021 Intel Corporation. All rights reserved.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 *  version: QAT.L.4.15.0-00011
 *
 ***************************************************************************/

/*
 * This is sample code that uses the primality testing APIs. A hard-coded
 * prime number is tested with four different algorithms:
 *
 * - GCD primality test
 * - Fermat primality test
 * - Miller-Rabin primality test. This test requires random numbers that are
 *   also hardcoded here (see unsigned char MR[])
 * - Lucas primality test
 */

#include "cpa.h"
#include "cpa_cy_im.h"
//#include "cpa_cy_prime.h"
#include "cpa_cy_ln.h"

#include <time.h>
#include <openssl/bn.h>

#include "cpa_sample_utils.h"

#define NB_MR_ROUNDS 2
#define TIMEOUT_MS 5000 /* 5 seconds*/

extern int gDebugParam;

/* Sample prime number: we want to test the primality of this number */
//static Cpa8U sampleBase_1024[] = { 
//    0x07, 0x3A, 0xD3, 0x1F, 0x2B, 0x41, 0xAD, 0x36,
//    0x23, 0xDD, 0xD0, 0x47, 0x8A, 0xB5, 0xC9, 0xD6,
//    0xC4, 0x5B, 0x43, 0x4C, 0xE7, 0x74, 0x9A, 0xA1,
//    0x3D, 0x38, 0xAD, 0xC1, 0x7E, 0x7A, 0xD2, 0xF2,
//    0xD4, 0x6C, 0x6D, 0x87, 0x32, 0x52, 0x1D, 0xDA,
//    0x16, 0x1C, 0xCB, 0x2B, 0x2C, 0x1D, 0x8E, 0x29,
//    0xA6, 0x3F, 0xD9, 0x0C, 0xD4, 0xCE, 0x2C, 0x9C,
//    0x0F, 0xBE, 0x5D, 0x6E, 0x68, 0x5A, 0xBF, 0x7D,
//    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//    0x36, 0x61, 0xAD, 0x36, 0x36, 0x61, 0xAD, 0x36,
//    0x96, 0x43, 0xC9, 0xD6, 0x96, 0x43, 0xC9, 0xD6,
//    0x5A, 0xA9, 0x9A, 0xA1, 0x5A, 0xA9, 0x9A, 0xA1,
//    0x95, 0xB4, 0xD2, 0xF2, 0x95, 0xB4, 0xD2, 0xF2,
//    0xC8, 0xDF, 0x1D, 0xDA, 0xC8, 0xDF, 0x1D, 0xDA,
//    0x7C, 0x82, 0x8E, 0x29, 0x7C, 0x82, 0x8E, 0x29,
//    0x40, 0xC9, 0x2C, 0x9C, 0x40, 0xC9, 0x2C, 0x9C};

// 216 (in big endian format) 
static Cpa8U sampleBase_1024[] = { 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD8};

// 7 (in big endian format)
static Cpa8U sampleModulus_1024[] = { 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07};

// 3 (in big endian format)
static Cpa8U sampleExponent_1024[] = { 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};


/* Forward declaration */
CpaStatus lnModExpSample(void);
CpaStatus bnModExpSample(BIGNUM *r, BIGNUM *b, BIGNUM *e, BIGNUM *m, int nbits);

static clock_t qat_start;
static clock_t qat_elapsed;

/*
 * Callback function
 *
 * This function is "called back" (invoked by the implementation of
 * the API) when the asynchronous operation has completed.  The
 * context in which it is invoked depends on the implementation, but
 * as described in the API it should not sleep (since it may be called
 * in a context which does not permit sleeping, e.g. a Linux bottom
 * half).
 *
 * This function can perform whatever processing is appropriate to the
 * application.  For example, it may free memory, continue processing,
 * etc.  In this example, the function prints out the status of the
 * primality test, and sets the complete variable to indicate it has
 * been called.
 */
static void lnModExpCallback(void *pCallbackTag,
                          CpaStatus status,
                          void *pOpData,
                          CpaFlatBuffer *pOut)
{
    qat_elapsed = clock() - qat_start;

#ifdef _DESTINY_DEBUG_VERBOSE
    PRINT_DBG("lnModExpCallback, status = %d.\n", status);
    if (NULL == pOpData) {
        PRINT_ERR("pOpData is null, status = %d\n", status);
        return;
    } else {
	CpaCyLnModExpOpData *opData = (CpaCyLnModExpOpData *) pOpData;
        displayHexArray("\tbase: ",opData->base.pData,
	    	    opData->base.dataLenInBytes);
        displayHexArray("\texponent: ",opData->exponent.pData,
	    	    opData->exponent.dataLenInBytes);
        displayHexArray("\tmodulus: ",opData->modulus.pData,
	            opData->modulus.dataLenInBytes);
    }
#endif

    if (CPA_STATUS_SUCCESS != status) {
        PRINT_ERR("callback failed, status = %d\n", status);
    } else {
#ifdef _DESTINY_DEBUG_VERBOSE
        // 216^3 mod 7 = 6
        displayHexArray("\tb^e mod m: ", pOut->pData, pOut->dataLenInBytes);
#endif
        printf("QAT: %.1lfus\t", qat_elapsed / (CLOCKS_PER_SEC / 1000000.0));

        // The callback tag in this happens to be a synchronization object
        if (NULL != pCallbackTag) {
#ifdef _DESTINY_DEBUG_VERBOSE
            PRINT_DBG("Modular exponentiation completed.\n");
            PRINT_DBG("Wake up waiting thread from callback function.\n");
#endif
	    COMPLETE((struct COMPLETION_STRUCT *)pCallbackTag);
        }
    }

    return ;
}


static CpaStatus bnModExpPerformOp(BIGNUM *r, BIGNUM *b, BIGNUM *e, BIGNUM *m, int nbits, CpaInstanceHandle cyInstHandle)
{ 
    // nbits is the number of security bits
    int len = nbits / 8; // length in bytes

    CpaStatus status = CPA_STATUS_SUCCESS;

    //CpaBoolean testPassed = CPA_FALSE;

    // Mod Exp buffers 
    Cpa8U *pBase = NULL;
    Cpa8U *pModulus = NULL;
    Cpa8U *pExponent = NULL;
    CpaFlatBuffer modExpRes = {.pData = NULL, .dataLenInBytes = 0};

    struct COMPLETION_STRUCT complete;

#ifdef _DESTINY_DEBUG_VERBOSE
    PRINT_DBG("modExpPerformOp\n");
#endif

    COMPLETION_INIT(&complete);

    // Allocate device memory and copy data from host
    status = PHYS_CONTIG_ALLOC(&pBase, len);
    if (NULL != pBase) {
	unsigned char *bin = (unsigned char *) calloc(len, sizeof(unsigned char));
	if (BN_bn2binpad(b,bin,len)) {
            memcpy(pBase,bin,len);	
	} else {
	    PHYS_CONTIG_FREE(pBase);
        }
    }
    if (CPA_STATUS_SUCCESS == status) {
        status = PHYS_CONTIG_ALLOC(&pModulus, len);
	if (NULL != pModulus) {
	    unsigned char *bin = (unsigned char *) calloc(len, sizeof(unsigned char));
	    if (BN_bn2binpad(m,bin,len)) {
	        memcpy(pModulus,bin,len);
	    } else {
		PHYS_CONTIG_FREE(pModulus);
            }
	}
    }
    if (CPA_STATUS_SUCCESS == status) {
        status = PHYS_CONTIG_ALLOC(&pExponent, len);
        if (NULL != pExponent) {
	    unsigned char *bin = (unsigned char *) calloc(len, sizeof(unsigned char));
	    if (BN_bn2binpad(e,bin,len)) {
	        memcpy(pExponent,bin,len);
	    } else { 
                PHYS_CONTIG_FREE(pExponent);
	    }
        }
    }
    
    // Create test op data for the modular exponentiation 
    CpaCyLnModExpOpData modExpOpData = {
            .modulus = {.dataLenInBytes = 0, .pData = NULL},
            .base = {.dataLenInBytes = 0, .pData = NULL},
            .exponent = {.dataLenInBytes = 0, .pData = NULL}};

    // Allocate memory to store modular exponentiation results
    if (CPA_STATUS_SUCCESS == status) {
        status = PHYS_CONTIG_ALLOC(&modExpRes.pData, len);
        if (NULL != modExpRes.pData) {
            modExpRes.dataLenInBytes = len;
        } 
    }

#ifdef _DESTINY_DEBUG_VERBOSE
    printf("\tPerform %dth large number modular exponentiation.\n",i);
#endif

    if (CPA_STATUS_SUCCESS == status) {
        modExpOpData.base.dataLenInBytes = len; 
        modExpOpData.base.pData = pBase;
        modExpOpData.exponent.dataLenInBytes = len;
        modExpOpData.exponent.pData = pExponent;
        modExpOpData.modulus.dataLenInBytes = len;
        modExpOpData.modulus.pData = pModulus;

        qat_start = clock();	    
        status = cpaCyLnModExp(cyInstHandle,
                               lnModExpCallback, //NULL, /*callback function*/
                               (void *)&complete, //NULL, /*callback tag*/
                               &modExpOpData,
                               &modExpRes);
        
        if ((CPA_STATUS_RETRY != status) && (CPA_STATUS_SUCCESS != status)) {
            //if (CPA_STATUS_SUCCESS !=
            //    cpaCyGetStatusText(cyInstHandle, status, statusErrorString)) {
            //    PRINT_ERR("Error retrieving status string.\n");
            //}
#ifdef _DESTINY_DEBUG_VERBOSE
            PRINT_ERR("doModExp Fail -- "); //%s\n", statusErrorString);
#endif
	    status = CPA_STATUS_FAIL;
            //goto finish;
        }
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /** Wait until the callback function has been called*/
        if (!COMPLETION_WAIT(&complete, TIMEOUT_MS))
        {
#ifdef _DESTINY_DEBUG_VERBOSE
            PRINT_ERR("timeout or interruption in cpaCySymPerformOp\n");
#endif
	    status = CPA_STATUS_FAIL;
        } 
    }
 
    r = BN_bin2bn(modExpRes.pData, modExpRes.dataLenInBytes, r); 

    /** Free all allocated structures before exit*/
    PHYS_CONTIG_FREE(pBase);
    PHYS_CONTIG_FREE(pModulus);
    PHYS_CONTIG_FREE(pExponent);
    PHYS_CONTIG_FREE(modExpRes.pData);

    COMPLETION_DESTROY(&complete);

    return status;
}


/*
 * Perform a primality test operation on an hardcoded prime number
 */
static CpaStatus lnModExpPerformOp(CpaInstanceHandle cyInstHandle)
{
    CpaStatus status = CPA_STATUS_SUCCESS;

    //CpaBoolean testPassed = CPA_FALSE;

    // Mod Exp buffers 
    Cpa8U *pBase = NULL;
    Cpa8U *pModulus = NULL;
    Cpa8U *pExponent = NULL;
    CpaFlatBuffer modExpRes = {.pData = NULL, .dataLenInBytes = 0};

    struct COMPLETION_STRUCT complete;

    PRINT_DBG("modExpPerformOp\n");

    COMPLETION_INIT(&complete);

    // Allocate device memory and copy data from host
    status = PHYS_CONTIG_ALLOC(&pBase, sizeof(sampleBase_1024));
    if (NULL != pBase) {
        memcpy(pBase, sampleBase_1024, sizeof(sampleBase_1024));
    }
    if (CPA_STATUS_SUCCESS == status) {
        status = PHYS_CONTIG_ALLOC(&pModulus, sizeof(sampleModulus_1024));
        if (NULL != pModulus) {
            memcpy(pModulus, sampleModulus_1024, sizeof(sampleModulus_1024));
        }
    }
    if (CPA_STATUS_SUCCESS == status) {
        status = PHYS_CONTIG_ALLOC(&pExponent, sizeof(sampleExponent_1024));
        if (NULL != pExponent) {
            memcpy(pExponent, sampleExponent_1024, sizeof(sampleExponent_1024));
        }
    }
    
    // Create test op data for the modular exponentiation 
    //if (CPA_STATUS_SUCCESS == status) {
    //    status = OS_MALLOC(&pModExpTestOpData, sizeof(CpaCyPrimeTestOpData));
    //}
    CpaCyLnModExpOpData modExpOpData = {
            .modulus = {.dataLenInBytes = 0, .pData = NULL},
            .base = {.dataLenInBytes = 0, .pData = NULL},
            .exponent = {.dataLenInBytes = 0, .pData = NULL}};

    // Allocate memory to store modular exponentiation results
    if (CPA_STATUS_SUCCESS == status) {
        status = PHYS_CONTIG_ALLOC(&modExpRes.pData, sizeof(sampleModulus_1024));
        if (NULL != modExpRes.pData) {
            modExpRes.dataLenInBytes = sizeof(sampleModulus_1024);
        } 
    }

    int i = 0;
    //for (i = 0; i < 10; i++) {
        printf("\tPerform %dth large number modular exponentiation.\n",i);
        if (CPA_STATUS_SUCCESS == status) {
            modExpOpData.base.dataLenInBytes = sizeof(sampleBase_1024); 
            modExpOpData.base.pData = pBase;
            modExpOpData.exponent.dataLenInBytes = sizeof(sampleExponent_1024);
            modExpOpData.exponent.pData = pExponent;
            modExpOpData.modulus.dataLenInBytes = sizeof(sampleModulus_1024);
            modExpOpData.modulus.pData = pModulus;

            displayHexArray("--base: ",pBase,//modExpOpData.base.pData,
			    modExpOpData.base.dataLenInBytes);
            displayHexArray("--exponent: ",pExponent,//modExpOpData.exponent.pData,
			    modExpOpData.exponent.dataLenInBytes);
            displayHexArray("--modulus: ",pModulus,//modExpOpData.modulus.pData,
			    modExpOpData.modulus.dataLenInBytes);
            
	    
	    status = cpaCyLnModExp(cyInstHandle,
                                   lnModExpCallback, //NULL, /*callback function*/
                                   (void *)&complete, //NULL, /*callback tag*/
                                   &modExpOpData,
                                   &modExpRes);
            
            if ((CPA_STATUS_RETRY != status) && (CPA_STATUS_SUCCESS != status)) {
                //if (CPA_STATUS_SUCCESS !=
                //    cpaCyGetStatusText(cyInstHandle, status, statusErrorString)) {
                //    PRINT_ERR("Error retrieving status string.\n");
                //}
                PRINT_ERR("doModExp Fail -- "); //%s\n", statusErrorString);
                status = CPA_STATUS_FAIL;
                //goto finish;
            }
        }

        if (CPA_STATUS_SUCCESS == status)
        {
            /** Wait until the callback function has been called*/
            if (!COMPLETION_WAIT(&complete, TIMEOUT_MS))
            {
                PRINT_ERR("timeout or interruption in cpaCySymPerformOp\n");
                status = CPA_STATUS_FAIL;
            } else {
                
                displayHexArray("--modExpRes: ",modExpRes.pData,modExpRes.dataLenInBytes);
            }
        }
    //}

    displayHexArray("++modExpRes: ",modExpRes.pData,modExpRes.dataLenInBytes);
    
    /** Free all allocated structures before exit*/
    PHYS_CONTIG_FREE(pBase);
    PHYS_CONTIG_FREE(pModulus);
    PHYS_CONTIG_FREE(pExponent);

    COMPLETION_DESTROY(&complete);

    return status;
}


CpaStatus lnModExpSample(void)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    CpaInstanceHandle cyInstHandle = NULL;
    //CpaCyPrimeStats64 primeStats = {0};
    CpaCyLnStats64 lnStats = {0};

    PRINT_DBG("start of modExp sample code\n");

    /*
     * In this simplified version of instance discovery, we discover
     * exactly one instance of a crypto service.
     */
    sampleCyGetInstance(&cyInstHandle);
    if (cyInstHandle == NULL) {
        return CPA_STATUS_FAIL;
    }

    /* Start Cryptographic component */
    PRINT_DBG("cpaCyStartInstance\n");
    status = cpaCyStartInstance(cyInstHandle);

    if (CPA_STATUS_SUCCESS == status) {
        // Set the address translation function for the instance
        status = cpaCySetAddressTranslation(cyInstHandle, sampleVirtToPhys);
    }

    if (CPA_STATUS_SUCCESS == status) {
        /*
         * If the instance is polled start the polling thread. Note that
         * how the polling is done is implementation-dependent.
         */
        sampleCyStartPolling(cyInstHandle);

        /** Perform large number modular exponentiation test operations */
        status = lnModExpPerformOp(cyInstHandle);
    }

    if (CPA_STATUS_SUCCESS == status) {
        PRINT_DBG("cpaCyLnStatsQuery\n");
        //status = cpaCyPrimeQueryStats64(cyInstHandle, &primeStats);
        status = cpaCyLnStatsQuery64(cyInstHandle, &lnStats);
        if (status != CPA_STATUS_SUCCESS) {
            PRINT_ERR("cpaCyLnStatsQuery() failed. (status = %d)\n", status);
        }
        PRINT_DBG("Number of lnModExp requests: %llu\n",
                  (unsigned long long)lnStats.numLnModExpRequests);
    }

    /* Stop the polling thread */
    sampleCyStopPolling();

    /** Stop Cryptographic component */
    PRINT_DBG("cpaCyStopInstance\n");
    status = cpaCyStopInstance(cyInstHandle);

    if (CPA_STATUS_SUCCESS == status)
    {
        PRINT_DBG("Sample code ran successfully\n");
    }
    else
    {
        PRINT_DBG("Sample code failed with status of %d\n", status);
    }

    return status;
}


CpaStatus bnModExpSample(BIGNUM *r, BIGNUM *b, BIGNUM *e, BIGNUM *m, int nbits)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    CpaInstanceHandle cyInstHandle = NULL;
    //CpaCyPrimeStats64 primeStats = {0};
    CpaCyLnStats64 lnStats = {0};

#ifdef _DESTINY_DEBUG_VERBOSE
    PRINT_DBG("start of bnModExp sample code\n");
#endif

    /*
     * In this simplified version of instance discovery, we discover
     * exactly one instance of a crypto service.
     */
    sampleCyGetInstance(&cyInstHandle);
    if (cyInstHandle == NULL) {
        return CPA_STATUS_FAIL;
    }

#ifdef _DESTINY_DEBUG_VERBOSE
    /* Start Cryptographic component */
    PRINT_DBG("cpaCyStartInstance\n");
#endif
    status = cpaCyStartInstance(cyInstHandle);

    if (CPA_STATUS_SUCCESS == status) {
        // Set the address translation function for the instance
        status = cpaCySetAddressTranslation(cyInstHandle, sampleVirtToPhys);
    }

    if (CPA_STATUS_SUCCESS == status) {
        /*
         * If the instance is polled start the polling thread. Note that
         * how the polling is done is implementation-dependent.
         */
        sampleCyStartPolling(cyInstHandle);

        /** Perform big number modular exponentiation test operation */
        status = bnModExpPerformOp(r, b, e, m, nbits, cyInstHandle);
    }

    if (CPA_STATUS_SUCCESS == status) {
#ifdef _DESTINY_DEBUG_VERBOSE
        PRINT_DBG("cpaCyLnStatsQuery\n");
#endif
	//status = cpaCyPrimeQueryStats64(cyInstHandle, &primeStats);
        status = cpaCyLnStatsQuery64(cyInstHandle, &lnStats);
        if (status != CPA_STATUS_SUCCESS) {
            PRINT_ERR("cpaCyLnStatsQuery() failed. (status = %d)\n", status);
        }
#ifdef _DESTINY_DEBUG_VERBOSE
        PRINT_DBG("Number of bnModExp requests: %llu\n",
                  (unsigned long long)lnStats.numLnModExpRequests);
#endif
    }

    /* Stop the polling thread */
    sampleCyStopPolling();

#ifdef _DESTINY_DEBUG_VERBOSE
    /** Stop Cryptographic component */
    PRINT_DBG("cpaCyStopInstance\n");
#endif
    status = cpaCyStopInstance(cyInstHandle);

#ifdef _DESTINY_DEBUG_VERBOSE
    if (CPA_STATUS_SUCCESS == status) {
        PRINT_DBG("Sample code ran successfully\n");
    } else {
        PRINT_DBG("Sample code failed with status of %d\n", status);
    }
#endif
    return status;
}

