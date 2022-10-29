// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "heqat/common/utils.h"
#include "heqat/common/types.h"

#include <openssl/err.h>
#include <openssl/rand.h>

BIGNUM* generateTestBNData(int nbits) {
    if (!RAND_status()) return NULL;

    HE_QAT_PRINT_DBG("PRNG properly seeded.\n");

    BIGNUM* bn = BN_new();

    if (!BN_rand(bn, nbits, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY)) {
        BN_free(bn);
        HE_QAT_PRINT_ERR("Error while generating BN random number: %lu\n",
                         ERR_get_error());
        return NULL;
    }

    return bn;
}

unsigned char* paddingZeros(BIGNUM* bn, int nbits) {
    if (!bn) return NULL;

    // Returns same address if it fails
    int num_bytes = BN_num_bytes(bn);
    int bytes_left = nbits / 8 - num_bytes;
    if (bytes_left <= 0) return NULL;

    // Returns same address if it fails
    unsigned char* bin = NULL;
    int len = bytes_left + num_bytes;
    if (!(bin = (unsigned char*)OPENSSL_zalloc(len))) return NULL;

    HE_QAT_PRINT_DBG("Padding bn with %d bytes to total %d bytes\n", bytes_left,
                     len);

    BN_bn2binpad(bn, bin, len);
    if (ERR_get_error()) {
        OPENSSL_free(bin);
        return NULL;
    }

    return bin;
}

void showHexBN(BIGNUM* bn, int nbits) {
    int len = nbits / 8;
    unsigned char* bin = (unsigned char*)OPENSSL_zalloc(len);
    if (!bin) return;
    if (BN_bn2binpad(bn, bin, len)) {
        for (size_t i = 0; i < len; i++) HE_QAT_PRINT("%2.2x", bin[i]);
        HE_QAT_PRINT("\n");
    }
    OPENSSL_free(bin);
    return;
}

void showHexBin(unsigned char* bin, int len) {
    if (!bin) return;
    for (size_t i = 0; i < len; i++) HE_QAT_PRINT("%2.2x", bin[i]);
    HE_QAT_PRINT("\n");
    return;
}
