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

#ifndef _BIGNUM_H_
#define _BIGNUM_H_

#include <ippcp.h>

#include <ostream>
#include <vector>

class BigNumber {
 public:
  BigNumber(Ipp32u value = 0);
  BigNumber(Ipp32s value);
  BigNumber(const IppsBigNumState* pBN);
  BigNumber(const Ipp32u* pData, int length = 1,
            IppsBigNumSGN sgn = IppsBigNumPOS);
  BigNumber(const BigNumber& bn);
  BigNumber(const char* s);
  virtual ~BigNumber();
  const void* addr = static_cast<const void*>(this);

  // set value
  void Set(const Ipp32u* pData, int length = 1,
           IppsBigNumSGN sgn = IppsBigNumPOS);
  // conversion to IppsBigNumState
  friend IppsBigNumState* BN(const BigNumber& bn) { return bn.m_pBN; }
  operator IppsBigNumState*() const { return m_pBN; }

  // some useful constants
  static const BigNumber& Zero();
  static const BigNumber& One();
  static const BigNumber& Two();

  // arithmetic operators probably need
  BigNumber& operator=(const BigNumber& bn);
  // Bin: Support integer add
  BigNumber& operator+=(Ipp32u n);
  BigNumber& operator+=(const BigNumber& bn);
  // Bin: Support integer sub
  BigNumber& operator-=(Ipp32u n);
  BigNumber& operator-=(const BigNumber& bn);
  // Bin: Support integer mul
  BigNumber& operator*=(Ipp32u n);
  BigNumber& operator*=(const BigNumber& bn);
  // Bin: Support integer div
  BigNumber& operator/=(Ipp32u n);
  BigNumber& operator/=(const BigNumber& bn);
  // Bin: Support integer mod
  BigNumber& operator%=(Ipp32u n);
  BigNumber& operator%=(const BigNumber& bn);
  friend BigNumber operator+(const BigNumber& a, const BigNumber& b);
  // Bin: Support integer add
  friend BigNumber operator+(const BigNumber& a, Ipp32u);
  friend BigNumber operator-(const BigNumber& a, const BigNumber& b);
  // Bin: Support integer sub
  friend BigNumber operator-(const BigNumber& a, Ipp32u);
  friend BigNumber operator*(const BigNumber& a, const BigNumber& b);
  // Bin: Support integer mul
  friend BigNumber operator*(const BigNumber& a, Ipp32u);
  friend BigNumber operator%(const BigNumber& a, const BigNumber& b);
  // Bin: Support integer mod
  friend BigNumber operator%(const BigNumber& a, Ipp32u);
  friend BigNumber operator/(const BigNumber& a, const BigNumber& b);
  // Bin: Support integer div
  friend BigNumber operator/(const BigNumber& a, Ipp32u);

  // modulo arithmetic
  BigNumber Modulo(const BigNumber& a) const;
  BigNumber ModAdd(const BigNumber& a, const BigNumber& b) const;
  BigNumber ModSub(const BigNumber& a, const BigNumber& b) const;
  BigNumber ModMul(const BigNumber& a, const BigNumber& b) const;
  BigNumber InverseAdd(const BigNumber& a) const;
  BigNumber InverseMul(const BigNumber& a) const;
  BigNumber gcd(const BigNumber& q) const;
  int compare(const BigNumber&) const;

  // comparisons
  friend bool operator<(const BigNumber& a, const BigNumber& b);
  friend bool operator>(const BigNumber& a, const BigNumber& b);
  friend bool operator==(const BigNumber& a, const BigNumber& b);
  friend bool operator!=(const BigNumber& a, const BigNumber& b);
  friend bool operator<=(const BigNumber& a, const BigNumber& b) {
    return !(a > b);
  }
  friend bool operator>=(const BigNumber& a, const BigNumber& b) {
    return !(a < b);
  }

  // easy tests
  bool IsOdd() const;
  bool IsEven() const { return !IsOdd(); }
  // Bin: Support TestBit
  bool TestBit(int index) const;

  // size of BigNumber
  int MSB() const;
  int LSB() const;
  int BitSize() const { return MSB() + 1; }
  int DwordSize() const { return (BitSize() + 31) >> 5; }
  friend int Bit(const std::vector<Ipp32u>& v, int n);

  // conversion and output
  void num2hex(std::string& s) const;          // convert to hex string
  void num2vec(std::vector<Ipp32u>& v) const;  // convert to 32-bit word vector
  friend std::ostream& operator<<(std::ostream& os, const BigNumber& a);
  void num2char(std::vector<Ipp8u>& dest) const;

 protected:
  bool create(const Ipp32u* pData, int length,
              IppsBigNumSGN sgn = IppsBigNumPOS);
  IppsBigNumState* m_pBN;
};

constexpr int BITSIZE_WORD(int n) { return (((n) + 31) >> 5); }
constexpr int BITSIZE_DWORD(int n) { return (((n) + 63) >> 6); }

#endif  // _BIGNUM_H_
