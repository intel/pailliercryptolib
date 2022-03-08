
#include "cpa_sample_utils.h"

#include "he_qat_bn_ops.h"
#include "he_qat_context.h"

#include <time.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <string>

#include "ippcp.h"
#include "bignum.h"

#include <iostream>
#include <iomanip>

#define LEN_OF_1024_BITS 128
#define LEN_OF_2048_BITS 256
#define msb_CAN_BE_ZERO -1
#define msb_IS_ONE 0
#define EVEN_RND_NUM 0
#define ODD_RND_NUM 1
#define BATCH_SIZE 1

using namespace std;

//int gDebugParam = 1;

BIGNUM* generateTestBNData(int nbits) {
    if (!RAND_status()) return NULL;
#ifdef _DESTINY_DEBUG_VERBOSE
    printf("PRNG properly seeded.\n");
#endif

    BIGNUM* bn = BN_new();

    if (!BN_rand(bn, nbits, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY)) {
        BN_free(bn);
        printf("Error while generating BN random number: %lu\n",
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
    if (!(bin = OPENSSL_zalloc(len))) return NULL;

#ifdef _DESTINY_DEBUG_VERBOSE
    printf("Padding bn with %d bytes to total %d bytes\n", bytes_left, len);
#endif
    BN_bn2binpad(bn, bin, len);
    if (ERR_get_error()) {
        OPENSSL_free(bin);
        return NULL;
    }

    return bin;
}

void showHexBN(BIGNUM* bn, int nbits) {
    int len = nbits / 8;
    unsigned char* bin = OPENSSL_zalloc(len);
    if (!bin) return;
    if (BN_bn2binpad(bn, bin, len)) {
        for (size_t i = 0; i < len; i++) printf("%d", bin[i]);
        printf("\n");
    }
    OPENSSL_free(bin);
    return;
}

void showHexBin(unsigned char* bin, int len) {
    if (!bin) return;
    for (size_t i = 0; i < len; i++) printf("%d", bin[i]);
    printf("\n");
    return;
}

// data will be changed to little endian format in this function, therefore the abscence of const in front
HE_QAT_STATUS binToBigNumber(BigNumber &bn, unsigned char *data, int nbits)
{
    HE_QAT_STATUS status = HE_QAT_STATUS_FAIL;

    if (nbits <= 0) return HE_QAT_STATUS_INVALID_PARAM;

    BIGNUM *bn_ = BN_new();
    if (ERR_get_error()) return status;
    
    // Convert raw data to BIGNUM representation to be used as a proxy to convert it to little endian format
    int len_ = nbits/8;
    BN_bin2bn(data, len_, bn_);
    if (ERR_get_error()) {
        BN_free(bn_);
	return status;
    }
 
    // Convert big number from Big Endian (QAT's format) to Little Endian (BigNumber's format)
    BN_bn2lebinpad(bn_, data, len_);
    if (ERR_get_error()) {
        BN_free(bn_);
	return status;
    }

    // Uncomment this line if C++ compiler
    bn = BigNumber(reinterpret_cast<Ipp32u *>(data), BITSIZE_WORD(nbits));
    // Comment this line if C++ compiler
//    bn = BigNumber((Ipp32u *)(data), BITSIZE_WORD(nbits)); 

    // Set status of operation 
    status = HE_QAT_STATUS_SUCCESS;

    // Free up temporary allocated memory space
    BN_free(bn_);

    return status;
}

HE_QAT_STATUS bigNumberToBin(unsigned char *data, int nbits, BigNumber &bn)
{
    HE_QAT_STATUS status = HE_QAT_STATUS_FAIL;
    
    if (nbits <= 0) return HE_QAT_STATUS_INVALID_PARAM;
    
    int len_ = nbits/8;
    int bit_size_ = 0; // could use that instead of nbits
    Ipp32u *ref_bn_ = NULL; 
    ippsRef_BN(NULL, &bit_size_, &ref_bn_, BN(bn));
    unsigned char *data_ = reinterpret_cast<unsigned char *>(ref_bn_);
//  unsigned char *data = (unsigned char *) ref_bn_;
    
    BIGNUM *bn_ = BN_new();
    if (ERR_get_error()) return status;
    
    BN_lebin2bn(data_, len_, bn_);
    if (ERR_get_error()) {
        BN_free(bn_);
	return status;
    }

    BN_bn2binpad(bn_, data, len_);
    if (ERR_get_error()) {
        BN_free(bn_);
	return status;
    }

    // Set status of operation 
    status = HE_QAT_STATUS_SUCCESS;

    // Free up temporary allocated memory space
    BN_free(bn_);

    return status;
}

int main(int argc, const char** argv) {
    const int bit_length = 1024;
    const size_t num_trials = 100;

    double avg_speed_up = 0.0;
    double ssl_avg_time = 0.0;
    double qat_avg_time = 0.0;

    clock_t start = CLOCKS_PER_SEC;
    clock_t ssl_elapsed = CLOCKS_PER_SEC;
    clock_t qat_elapsed = CLOCKS_PER_SEC;

    HE_QAT_STATUS status = HE_QAT_STATUS_FAIL;

    // Set up QAT runtime context
    acquire_qat_devices();

    // Set up OpenSSL context (as baseline)
    BN_CTX* ctx = BN_CTX_new();
    BN_CTX_start(ctx);

    for (unsigned int mod = 4094; mod < 4096; mod++) {
        //BIGNUM* bn_mod = BN_new(); 
	BIGNUM* bn_mod = generateTestBNData(bit_length);

        if (!bn_mod) continue;

	//BN_set_word(bn_mod, mod);

        char* bn_str = BN_bn2hex(bn_mod);
        printf("Generated modulus: %s num_bytes: %d num_bits: %d\n", bn_str,
               BN_num_bytes(bn_mod), BN_num_bits(bn_mod));
//        OPENSSL_free(bn_str);
       	
//	
//	// Pack BIGNUM (in base 2^8) into BigNumber (in base 2^32)
	unsigned char* mpi_base = NULL;
	mpi_base = (unsigned char *) calloc(bit_length/8, sizeof(unsigned char));
	if (NULL == mpi_base) {
            BN_free(bn_mod);
	    continue;
	}
	BN_bn2lebinpad(bn_mod, mpi_base, bit_length/8);
//        // revert it
//	for (int k = 0; k < bit_length/8; k++)
//            be_mpi_base[k] = mpi_base[bit_length/8-k-1];
//

	printf("Try convert to BigNumber len=%lu\n",bit_length/(8*sizeof(unsigned char)));
//	// Convert from base 2^8 to base 2^32
	Ipp32u *ipp_mpi_base = reinterpret_cast<Ipp32u *>(mpi_base);// + (rep_len - bit_length/8)); 
//	//Ipp32u *ipp_mpi_base = (Ipp32u *) (mpi_base); //(rep_len_2-128); 
        BigNumber BigNum_base((Ipp32u)0);
	BigNum_base = BigNumber((Ipp32u *) ipp_mpi_base, BITSIZE_WORD(bit_length));
	printf("BigNumber has been created.\n");
        // End of packing (this overhead does not exist in practice)

	std::string str;
	BigNum_base.num2hex(str);
	printf("BigNum_base num2hex output:  %s %d\n",str.c_str(),bit_length/8); 

//	// Extract raw data from BigNumber 2^32 and convert it to base 2^8
	int bitSize = 0;
	Ipp32u *pBigNum_base = NULL; //BN(BigNum_base);
	ippsRef_BN(NULL, &bitSize, &pBigNum_base, BN(BigNum_base));
	unsigned char *ippbn = reinterpret_cast<unsigned char *>(pBigNum_base);
//	unsigned char *ippbn = (unsigned char *) pBigNum_base;
	BIGNUM *ipp_ref_bn = BN_new();
	BN_lebin2bn(ippbn, bit_length/8, ipp_ref_bn);
	char *ipp_ref_bn_str = BN_bn2hex(ipp_ref_bn);
        printf(">>>> PRINT IPP ref bn: %s bitSize(%d)\n", ipp_ref_bn_str, BN_num_bits(ipp_ref_bn));
	//BIGNUM *ipp_ref_bn = BN_new();
	//BN_lebin2bn(ippbn, bit_length/8, ipp_ref_bn);

        BN_free(bn_mod);
//        BN_free(bn_base);
//        BN_free(bn_exponent);
    }

    // Tear down OpenSSL context
    BN_CTX_end(ctx);

    // Tear down QAT runtime context
    release_qat_devices();

    return (int)status;
}
