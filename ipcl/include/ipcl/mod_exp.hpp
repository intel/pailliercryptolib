// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef IPCL_INCLUDE_IPCL_MOD_EXP_HPP_
#define IPCL_INCLUDE_IPCL_MOD_EXP_HPP_

#include <vector>

#include "ipcl/bignum.h"

namespace ipcl {

/**
 * Hybrid mode type
 */
enum class HybridMode {
  OPTIMAL = 95,
  QAT = 100,
  PREF_QAT90 = 90,
  PREF_QAT80 = 80,
  PREF_QAT70 = 70,
  PREF_QAT60 = 60,
  HALF = 50,
  PREF_IPP60 = 40,
  PREF_IPP70 = 30,
  PREF_IPP80 = 20,
  PREF_IPP90 = 10,
  IPP = 0,
  UNDEFINED = -1
};

/**
 * Set hybrid mode
 * @param[in] mode The type of hybrid mode
 */
void setHybridMode(HybridMode mode);

/**
 * Set the number of mod exp operatiions
 * @param[in] Proportion calculated with QAT
 */
void setHybridRatio(float qat_ratio);

/**
 * Turn off hybrid mod exp.
 */
void setHybridOff();

/**
 * Get current hybrid qat ratio
 */
float getHybridRatio();

/**
 * Get current hybrid mode
 */
HybridMode getHybridMode();

/**
 * Check current hybrid mode is OPTIMAL
 */
bool isHybridOptimal();

/**
 * Modular exponentiation for multi BigNumber
 * @param[in] base base of the exponentiation
 * @param[in] exp pow of the exponentiation
 * @param[in] mod modular
 * @return the modular exponentiation result of type BigNumber
 */
std::vector<BigNumber> modExp(const std::vector<BigNumber>& base,
                              const std::vector<BigNumber>& exp,
                              const std::vector<BigNumber>& mod);
/**
 * Modular exponentiation for single BigNumber
 * @param[in] base base of the exponentiation
 * @param[in] exp pow of the exponentiation
 * @param[in] mod modular
 * @return the modular exponentiation result of type BigNumber
 */
BigNumber modExp(const BigNumber& base, const BigNumber& exp,
                 const BigNumber& mod);

/**
 * IPP modular exponentiation for multi buffer
 * @param[in] base base of the exponentiation
 * @param[in] exp pow of the exponentiation
 * @param[in] mod modular
 * @return the modular exponentiation result of type BigNumber
 */
std::vector<BigNumber> ippModExp(const std::vector<BigNumber>& base,
                                 const std::vector<BigNumber>& exp,
                                 const std::vector<BigNumber>& mod);

/**
 * IPP modular exponentiation for single buffer
 * @param[in] base base of the exponentiation
 * @param[in] exp pow of the exponentiation
 * @param[in] mod modular
 * @return the modular exponentiation result of type BigNumber
 */
BigNumber ippModExp(const BigNumber& base, const BigNumber& exp,
                    const BigNumber& mod);

/**
 * QAT modular exponentiation for multi BigNumber
 * @param[in] base base of the exponentiation
 * @param[in] exp pow of the exponentiation
 * @param[in] mod modular
 * @return the modular exponentiation result of type BigNumber
 */
std::vector<BigNumber> qatModExp(const std::vector<BigNumber>& base,
                                 const std::vector<BigNumber>& exp,
                                 const std::vector<BigNumber>& mod);

}  // namespace ipcl
#endif  // IPCL_INCLUDE_IPCL_MOD_EXP_HPP_
