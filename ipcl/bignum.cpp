/*******************************************************************************
 * Copyright 2019-2021 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

#include "ipcl/bignum.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

//////////////////////////////////////////////////////////////////////
//
// BigNumber
//
//////////////////////////////////////////////////////////////////////

BigNumber::~BigNumber() { delete[](Ipp8u*) m_pBN; }

bool BigNumber::create(const Ipp32u* pData, int length, IppsBigNumSGN sgn) {
  int size;
  ippsBigNumGetSize(length, &size);
  m_pBN = (IppsBigNumState*)(new Ipp8u[size]);
  if (!m_pBN) return false;
  ippsBigNumInit(length, m_pBN);
  if (pData) ippsSet_BN(sgn, length, pData, m_pBN);
  return true;
}

//
// constructors
//
BigNumber::BigNumber(Ipp32u value) { create(&value, 1, IppsBigNumPOS); }

BigNumber::BigNumber(Ipp32s value) {
  Ipp32s avalue = abs(value);
  create((Ipp32u*)&avalue, 1, (value < 0) ? IppsBigNumNEG : IppsBigNumPOS);
}

BigNumber::BigNumber(const IppsBigNumState* pBN) {
  IppsBigNumSGN bnSgn;
  int bnBitLen;
  Ipp32u* bnData;
  ippsRef_BN(&bnSgn, &bnBitLen, &bnData, pBN);

  create(bnData, BITSIZE_WORD(bnBitLen), bnSgn);
}

BigNumber::BigNumber(const Ipp32u* pData, int length, IppsBigNumSGN sgn) {
  create(pData, length, sgn);
}

// Bin: Following GMP lower case
static char HexDigitList[] = "0123456789abcdef";

BigNumber::BigNumber(const char* s) {
  bool neg = '-' == s[0];
  if (neg) s++;
  bool hex = ('0' == s[0]) && (('x' == s[1]) || ('X' == s[1]));

  int dataLen;
  Ipp32u base;
  if (hex) {
    s += 2;
    base = 0x10;
    dataLen = (int)(strlen(s) + 7) / 8;
  } else {
    base = 10;
    dataLen = (int)(strlen(s) + 9) / 10;
  }

  create(0, dataLen);
  *(this) = Zero();
  while (*s) {
    char tmp[2] = {s[0], 0};
    Ipp32u digit = (Ipp32u)strcspn(HexDigitList, tmp);
    *this = (*this) * base + BigNumber(digit);
    s++;
  }

  if (neg) (*this) = Zero() - (*this);
}

BigNumber::BigNumber(const BigNumber& bn) {
  IppsBigNumSGN bnSgn;
  int bnBitLen;
  Ipp32u* bnData;
  ippsRef_BN(&bnSgn, &bnBitLen, &bnData, bn);

  create(bnData, BITSIZE_WORD(bnBitLen), bnSgn);
}

//
// set value
//
void BigNumber::Set(const Ipp32u* pData, int length,
                    IppsBigNumSGN sgn) {  // = IppsBigNumPOS) {
  ippsSet_BN(sgn, length, pData, BN(*this));
}

//
// constants
//
const BigNumber& BigNumber::Zero() {
  static const BigNumber zero(0);
  return zero;
}

const BigNumber& BigNumber::One() {
  static const BigNumber one(1);
  return one;
}

const BigNumber& BigNumber::Two() {
  static const BigNumber two(2);
  return two;
}

//
// arithmetic operators
//
BigNumber& BigNumber::operator=(const BigNumber& bn) {
  if (this != &bn) {  // prevent self copy
    IppsBigNumSGN bnSgn;
    int bnBitLen;
    Ipp32u* bnData;
    ippsRef_BN(&bnSgn, &bnBitLen, &bnData, bn);

    delete[] (Ipp8u*)m_pBN;
    create(bnData, BITSIZE_WORD(bnBitLen), bnSgn);
  }
  return *this;
}

BigNumber& BigNumber::operator+=(const BigNumber& bn) {
  int aBitLen;
  ippsRef_BN(nullptr, &aBitLen, nullptr, *this);
  int bBitLen;
  ippsRef_BN(nullptr, &bBitLen, nullptr, bn);
  int rBitLen = IPP_MAX(aBitLen, bBitLen) + 1;

  BigNumber result(0, BITSIZE_WORD(rBitLen));
  ippsAdd_BN(*this, bn, result);
  *this = result;
  return *this;
}

// Bin: Support integer add
BigNumber& BigNumber::operator+=(Ipp32u n) {
  int aBitLen;
  ippsRef_BN(nullptr, &aBitLen, nullptr, *this);
  int rBitLen = IPP_MAX(aBitLen, sizeof(Ipp32u) * 8) + 1;

  BigNumber result(0, BITSIZE_WORD(rBitLen));
  BigNumber bn(n);
  ippsAdd_BN(*this, bn, result);
  *this = result;
  return *this;
}

BigNumber& BigNumber::operator-=(const BigNumber& bn) {
  int aBitLen;
  ippsRef_BN(nullptr, &aBitLen, nullptr, *this);
  int bBitLen;
  ippsRef_BN(nullptr, &bBitLen, nullptr, bn);
  int rBitLen = IPP_MAX(aBitLen, bBitLen);

  BigNumber result(0, BITSIZE_WORD(rBitLen));
  ippsSub_BN(*this, bn, result);
  *this = result;
  return *this;
}

// Bin: Support integer sub
BigNumber& BigNumber::operator-=(Ipp32u n) {
  int aBitLen;
  ippsRef_BN(nullptr, &aBitLen, nullptr, *this);
  int rBitLen = IPP_MAX(aBitLen, sizeof(Ipp32u) * 8);

  BigNumber result(0, BITSIZE_WORD(rBitLen));
  BigNumber bn(n);
  ippsSub_BN(*this, bn, result);
  *this = result;
  return *this;
}

BigNumber& BigNumber::operator*=(const BigNumber& bn) {
  int aBitLen;
  ippsRef_BN(nullptr, &aBitLen, nullptr, *this);
  int bBitLen;
  ippsRef_BN(nullptr, &bBitLen, nullptr, bn);
  int rBitLen = aBitLen + bBitLen;

  BigNumber result(0, BITSIZE_WORD(rBitLen));
  ippsMul_BN(*this, bn, result);
  *this = result;
  return *this;
}

// Bin: Support integer multiply
BigNumber& BigNumber::operator*=(Ipp32u n) {
  int aBitLen;
  ippsRef_BN(nullptr, &aBitLen, nullptr, *this);

  BigNumber result(0, BITSIZE_WORD(aBitLen + 32));
  BigNumber bn(n);
  ippsMul_BN(*this, bn, result);
  *this = result;
  return *this;
}

BigNumber& BigNumber::operator%=(const BigNumber& bn) {
  BigNumber remainder(bn);
  ippsMod_BN(BN(*this), BN(bn), BN(remainder));
  *this = remainder;
  return *this;
}

// Bin: Support integer mod
BigNumber& BigNumber::operator%=(Ipp32u n) {
  int aBitLen;
  ippsRef_BN(nullptr, &aBitLen, nullptr, *this);

  BigNumber result(0, BITSIZE_WORD(aBitLen));
  BigNumber bn(n);
  ippsMod_BN(*this, bn, result);
  *this = result;
  return *this;
}

BigNumber& BigNumber::operator/=(const BigNumber& bn) {
  BigNumber quotient(*this);
  BigNumber remainder(bn);
  ippsDiv_BN(BN(*this), BN(bn), BN(quotient), BN(remainder));
  *this = quotient;
  return *this;
}

// Bin: Support integer div
BigNumber& BigNumber::operator/=(Ipp32u n) {
  BigNumber quotient(*this);
  BigNumber bn(n);
  BigNumber remainder(bn);
  ippsDiv_BN(BN(*this), BN(bn), BN(quotient), BN(remainder));
  *this = quotient;
  return *this;
}

BigNumber operator+(const BigNumber& a, const BigNumber& b) {
  BigNumber r(a);
  return r += b;
}

// Bin: Support integer add
BigNumber operator+(const BigNumber& a, Ipp32u n) {
  BigNumber r(a);
  return r += n;
}

BigNumber operator-(const BigNumber& a, const BigNumber& b) {
  BigNumber r(a);
  return r -= b;
}

// Bin: Support integer sub
BigNumber operator-(const BigNumber& a, Ipp32u n) {
  BigNumber r(a);
  return r -= n;
}

BigNumber operator*(const BigNumber& a, const BigNumber& b) {
  BigNumber r(a);
  return r *= b;
}

// Bin: Support integer mul
BigNumber operator*(const BigNumber& a, Ipp32u n) {
  BigNumber r(a);
  return r *= n;
}

BigNumber operator/(const BigNumber& a, const BigNumber& b) {
  BigNumber q(a);
  return q /= b;
}

// Bin: Support integer div
BigNumber operator/(const BigNumber& a, Ipp32u n) {
  BigNumber q(a);
  return q /= n;
}

BigNumber operator%(const BigNumber& a, const BigNumber& b) {
  BigNumber r(b);
  ippsMod_BN(BN(a), BN(b), BN(r));
  return r;
}

// Bin: Support integer mod
BigNumber operator%(const BigNumber& a, Ipp32u n) {
  BigNumber r(a);
  BigNumber bn(n);
  ippsMod_BN(BN(a), BN(bn), BN(r));
  return r;
}

//
// modulo arithmetic
//
BigNumber BigNumber::Modulo(const BigNumber& a) const { return a % *this; }

BigNumber BigNumber::InverseAdd(const BigNumber& a) const {
  BigNumber t = Modulo(a);
  if (t == BigNumber::Zero())
    return t;
  else
    return *this - t;
}

BigNumber BigNumber::InverseMul(const BigNumber& a) const {
  BigNumber r(*this);
  ippsModInv_BN(BN(a), BN(*this), BN(r));
  return r;
}

BigNumber BigNumber::ModAdd(const BigNumber& a, const BigNumber& b) const {
  BigNumber r = this->Modulo(a + b);
  return r;
}

BigNumber BigNumber::ModSub(const BigNumber& a, const BigNumber& b) const {
  BigNumber r = this->Modulo(a + this->InverseAdd(b));
  return r;
}

BigNumber BigNumber::ModMul(const BigNumber& a, const BigNumber& b) const {
  BigNumber r = this->Modulo(a * b);
  return r;
}

BigNumber BigNumber::gcd(const BigNumber& q) const {
  BigNumber gcd(*this);

  ippsGcd_BN(BN(*this), BN(q), BN(gcd));

  return gcd;
}

//
// comparison
//
int BigNumber::compare(const BigNumber& bn) const {
  Ipp32u result;
  BigNumber tmp = *this - bn;
  ippsCmpZero_BN(BN(tmp), &result);
  return (result == IS_ZERO) ? 0 : (result == GREATER_THAN_ZERO) ? 1 : -1;
}

bool operator<(const BigNumber& a, const BigNumber& b) {
  return a.compare(b) < 0;
}
bool operator>(const BigNumber& a, const BigNumber& b) {
  return a.compare(b) > 0;
}
bool operator==(const BigNumber& a, const BigNumber& b) {
  return 0 == a.compare(b);
}
bool operator!=(const BigNumber& a, const BigNumber& b) {
  return 0 != a.compare(b);
}

// easy tests
//
bool BigNumber::IsOdd() const {
  Ipp32u* bnData;
  ippsRef_BN(nullptr, nullptr, &bnData, *this);
  return bnData[0] & 1;
}

// Bin: Support TestBit
bool BigNumber::TestBit(int index) const {
  int bnIndex = index / 32;
  int shift = index % 32;
  Ipp32u* bnData;
  int aBitLen;
  ippsRef_BN(nullptr, &aBitLen, &bnData, *this);
  if (index > aBitLen)
    return 0;
  else
    return (bnData[bnIndex] >> shift) & 1;
}

//
// size of BigNumber
//
int BigNumber::LSB() const {
  if (*this == BigNumber::Zero()) return 0;

  std::vector<Ipp32u> v;
  num2vec(v);

  int lsb = 0;
  std::vector<Ipp32u>::iterator i;
  for (i = v.begin(); i != v.end(); i++) {
    Ipp32u x = *i;
    if (0 == x)
      lsb += 32;
    else {
      while (0 == (x & 1)) {
        lsb++;
        x >>= 1;
      }
      break;
    }
  }
  return lsb;
}

int BigNumber::MSB() const {
  if (*this == BigNumber::Zero()) return 0;

  std::vector<Ipp32u> v;
  num2vec(v);

  int msb = (int)v.size() * 32 - 1;
  std::vector<Ipp32u>::reverse_iterator i;
  for (i = v.rbegin(); i != v.rend(); i++) {
    Ipp32u x = *i;
    if (0 == x)
      msb -= 32;
    else {
      while (!(x & 0x80000000)) {
        msb--;
        x <<= 1;
      }
      break;
    }
  }
  return msb;
}

int Bit(const std::vector<Ipp32u>& v, int n) {
  return 0 != (v[n >> 5] & (1 << (n & 0x1F)));
}

//
// conversions and output
//
void BigNumber::num2vec(std::vector<Ipp32u>& v) const {
  int bnBitLen;
  Ipp32u* bnData;
  ippsRef_BN(nullptr, &bnBitLen, &bnData, *this);

  int len = BITSIZE_WORD(bnBitLen);
  ;
  for (int n = 0; n < len; n++) v.push_back(bnData[n]);
}

void BigNumber::num2hex(std::string& s) const {
  IppsBigNumSGN bnSgn;
  int bnBitLen;
  Ipp32u* bnData;
  ippsRef_BN(&bnSgn, &bnBitLen, &bnData, *this);

  int len = BITSIZE_WORD(bnBitLen);

  s.append(1, (bnSgn == ippBigNumNEG) ? '-' : ' ');
  s.append(1, '0');
  s.append(1, 'x');

  // remove zero value head
  int first_no_zero = 0;
  for (int n = len; n > 0; n--) {
    Ipp32u x = bnData[n - 1];
    for (int nd = 8; nd > 0; nd--) {
      char c = HexDigitList[(x >> (nd - 1) * 4) & 0xF];
      if (c != '0' || first_no_zero) {
        first_no_zero = 1;
        s.append(1, c);
      }
    }
  }
}

std::ostream& operator<<(std::ostream& os, const BigNumber& a) {
  std::string s;
  a.num2hex(s);
  os << s;
  return os;
}

void BigNumber::num2char(std::vector<Ipp8u>& dest) const {
  int bnBitLen;
  unsigned char* bnData;
  ippsRef_BN(nullptr, &bnBitLen, reinterpret_cast<Ipp32u**>(&bnData), *this);
  int len = (bnBitLen + 7) >> 3;
  dest.assign(bnData, bnData + len);
}

bool BigNumber::fromBin(BigNumber& bn, const unsigned char* data, int len) {
  if (len <= 0) return false;

  // Create BigNumber containg input data passed as argument
  bn = BigNumber(reinterpret_cast<const Ipp32u*>(data), (len / 4));
  Ipp32u* ref_bn_data_ = NULL;
  ippsRef_BN(NULL, NULL, &ref_bn_data_, BN(bn));

  // Convert it to little endian format
  unsigned char* data_ = reinterpret_cast<unsigned char*>(ref_bn_data_);
  for (int i = 0; i < len; i++) data_[i] = data[len - 1 - i];

  return true;
}

bool BigNumber::toBin(unsigned char* data, int len, const BigNumber& bn) {
  if (len <= 0) return false;

  // Extract raw vector of data in little endian format
  int bitSize = 0;
  Ipp32u* ref_bn_data_ = NULL;
  ippsRef_BN(NULL, &bitSize, &ref_bn_data_, BN(bn));

  // Revert it to big endian format
  int bitSizeLen = BITSIZE_WORD(bitSize) * 4;
  unsigned char* data_ = reinterpret_cast<unsigned char*>(ref_bn_data_);
  for (int i = 0; i < bitSizeLen; i++) data[len - 1 - i] = data_[i];

  return true;
}

bool BigNumber::toBin(unsigned char** bin, int* len, const BigNumber& bn) {
  if (NULL == bin) return false;
  if (NULL == len) return false;

  // Extract raw vector of data in little endian format
  int bitSize = 0;
  Ipp32u* ref_bn_data_ = NULL;
  ippsRef_BN(NULL, &bitSize, &ref_bn_data_, BN(bn));

  // Revert it to big endian format
  int bitSizeLen = BITSIZE_WORD(bitSize) * 4;
  *len = bitSizeLen;
  bin[0] = reinterpret_cast<unsigned char*>(
      malloc(bitSizeLen * sizeof(unsigned char)));
  memset(bin[0], 0, *len);
  if (NULL == bin[0]) return false;

  unsigned char* data_out = bin[0];
  unsigned char* bn_data_ = reinterpret_cast<unsigned char*>(ref_bn_data_);
  for (int i = 0; i < bitSizeLen; i++)
    data_out[bitSizeLen - 1 - i] = bn_data_[i];

  return true;
}
