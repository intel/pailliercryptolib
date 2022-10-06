// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <vector>

#include "ipcl/ipcl.hpp"

namespace ipcl {

constexpr int N_BIT_SIZE_MAX = 2048;
constexpr int N_BIT_SIZE_MIN = 200;

BigNumber getPrimeBN(int max_bits) {
  int prime_size;
  ippsPrimeGetSize(max_bits, &prime_size);
  auto prime_ctx = std::vector<Ipp8u>(prime_size);
  ippsPrimeInit(max_bits, reinterpret_cast<IppsPrimeState*>(prime_ctx.data()));

#if defined(IPCL_RNG_INSTR_RDSEED) || defined(IPCL_RNG_INSTR_RDRAND)
  bool rand_param = NULL;
#else
  auto buff = std::vector<Ipp8u>(prime_size);
  auto rand_param = buff.data();
  ippsPRNGInit(160, reinterpret_cast<IppsPRNGState*>(rand_param));
#endif

  BigNumber prime_bn(0, max_bits / 8);
  while (ippStsNoErr !=
         ippsPrimeGen_BN(prime_bn, max_bits, 10,
                         reinterpret_cast<IppsPrimeState*>(prime_ctx.data()),
                         ippGenRandom,
                         reinterpret_cast<IppsPRNGState*>(rand_param))) {
  }

  return prime_bn;
}

static BigNumber getPrimeDistance(int64_t key_size) {
  uint64_t count = key_size / 2 - 100;
  uint64_t ct32 = count / 32;   // number of 2**32 needed
  uint64_t res = count & 0x1F;  // count % 32
  std::vector<Ipp32u> tmp(ct32 + 1, 0);
  tmp[ct32] = 1 << res;

  BigNumber ref_dist(tmp.data(), tmp.size());
  return ref_dist;
}

static bool isClosePrimeBN(const BigNumber& p, const BigNumber& q,
                           const BigNumber& ref_dist) {
  BigNumber real_dist = (p >= q) ? (p - q) : (q - p);
  return (real_dist > ref_dist) ? false : true;
}

static void getNormalBN(int64_t n_length, BigNumber& p, BigNumber& q,
                        BigNumber& n, const BigNumber& ref_dist) {
  for (int64_t len = 0; (len != n_length) || isClosePrimeBN(p, q, ref_dist);
       len = n.BitSize()) {
    p = getPrimeBN(n_length / 2);
    q = p;
    while (q == p) {
      q = getPrimeBN(n_length / 2);
    }
    n = p * q;
  }
}

static void getDJNBN(int64_t n_length, BigNumber& p, BigNumber& q, BigNumber& n,
                     BigNumber& ref_dist) {
  BigNumber gcd;
  do {
    do {
      p = getPrimeBN(n_length / 2);
    } while (!p.TestBit(1));  // get p: p mod 4 = 3

    do {
      q = getPrimeBN(n_length / 2);
    } while (q == p || !p.TestBit(1));  // get q: q!=p and q mod 4 = 3

    gcd = (p - 1).gcd(q - 1);  // (p - 1) is a BigNumber
    n = p * q;
  } while ((gcd.compare(2)) || (n.BitSize() != n_length) ||
           isClosePrimeBN(p, q, ref_dist));  // gcd(p-1,q-1)=2
}

keyPair generateKeypair(int64_t n_length, bool enable_DJN) {
  /*
  https://www.intel.com/content/www/us/en/develop/documentation/ipp-crypto-reference/top/multi-buffer-cryptography-functions/modular-exponentiation/mbx-exp-1024-2048-3072-4096-mb8.html
  modulus size = n * n (keySize * keySize )
  */
  ERROR_CHECK(
      n_length <= N_BIT_SIZE_MAX,
      "generateKeyPair: modulus size in bits should belong to either 1Kb, 2Kb, "
      "3Kb or 4Kb range only, key size exceed the range!!!");
  ERROR_CHECK((n_length >= N_BIT_SIZE_MIN) && (n_length % 4 == 0),
              "generateKeyPair: key size should >=200, and divisible by 4");

  BigNumber ref_dist = getPrimeDistance(n_length);

  BigNumber p, q, n;

  if (enable_DJN)
    getDJNBN(n_length, p, q, n, ref_dist);
  else
    getNormalBN(n_length, p, q, n, ref_dist);

  PublicKey* public_key = new PublicKey(n, n_length, enable_DJN);
  PrivateKey* private_key = new PrivateKey(public_key, p, q);

  return keyPair{public_key, private_key};
}

}  // namespace ipcl
