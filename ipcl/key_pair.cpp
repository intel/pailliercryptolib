// Copyright (C) 2021-2022 Intel Corporation

#include "ipcl/key_pair.hpp"

#include <climits>
#include <random>
#include <vector>

static inline void rand32u(vector<Ipp32u>& addr) {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);
  for (auto& x : addr) x = (dist(rng) << 16) + dist(rng);
}

BigNumber getPrimeBN(int maxBitSize) {
  int PrimeSize;
  ippsPrimeGetSize(maxBitSize, &PrimeSize);
  auto primeGen = vector<Ipp8u>(PrimeSize);
  ippsPrimeInit(maxBitSize, reinterpret_cast<IppsPrimeState*>(primeGen.data()));

  // define Pseudo Random Generator (default settings)
  int seedBitSize = 160;
  int seedSize = BITSIZE_WORD(seedBitSize);

  ippsPRNGGetSize(&PrimeSize);
  auto rand = vector<Ipp8u>(PrimeSize);
  ippsPRNGInit(seedBitSize, reinterpret_cast<IppsPRNGState*>(rand.data()));

  auto seed = vector<Ipp32u>(seedSize);
  rand32u(seed);
  BigNumber bseed(seed.data(), seedSize, IppsBigNumPOS);

  ippsPRNGSetSeed(BN(bseed), reinterpret_cast<IppsPRNGState*>(rand.data()));

  // generate maxBit prime
  BigNumber pBN(0, maxBitSize / 8);
  while (ippStsNoErr !=
         ippsPrimeGen_BN(pBN, maxBitSize, 10,
                         reinterpret_cast<IppsPrimeState*>(primeGen.data()),
                         ippsPRNGen,
                         reinterpret_cast<IppsPRNGState*>(rand.data()))) {
  }

  return pBN;
}

inline void getNormalBN(int64_t n_length, BigNumber& p, BigNumber& q,
                        BigNumber& n) {
  for (int64_t len = 0; len != n_length; len = n.BitSize()) {
    p = getPrimeBN(n_length / 2);
    q = p;
    while (q == p) {
      q = getPrimeBN(n_length / 2);
    }
    n = p * q;
  }
}

inline void getDJNBN(int64_t n_length, BigNumber& p, BigNumber& q,
                     BigNumber& n) {
  BigNumber gcd;
  do {
    do {
      p = getPrimeBN(n_length / 2);
    } while (!p.TestBit(1));  // get p: p mod 4 = 3

    do {
      q = getPrimeBN(n_length / 2);
    } while (q == p || !p.TestBit(1));  // get q: q!=p and q mod 4 = 3

    BigNumber pminusone = p - 1;
    BigNumber qminusone = q - 1;
    gcd = pminusone.gcd(qminusone);
  } while (gcd.compare(2));  // gcd(p-1,q-1)=2

  n = p * q;
}

keyPair generateKeypair(int64_t n_length, bool enable_DJN) {
  /*
  https://www.intel.com/content/www/us/en/develop/documentation/ipp-crypto-reference/top/multi-buffer-cryptography-functions/modular-exponentiation/mbx-exp-1024-2048-3072-4096-mb8.html
  modulus size = n * n (keySize * keySize )
  */
  if (n_length > 2048) {
    throw std::runtime_error(
        "modulus size in bits should belong to either 1Kb, 2Kb, "
        "3Kb or 4Kb range only, key size exceed the range!!!");
  }

  BigNumber p, q, n;

  if (enable_DJN)
    getDJNBN(n_length, p, q, n);
  else
    getNormalBN(n_length, p, q, n);

  PaillierPublicKey* public_key =
      new PaillierPublicKey(n, n_length, enable_DJN);
  PaillierPrivateKey* private_key = new PaillierPrivateKey(public_key, p, q);

  return keyPair{public_key, private_key};
}
