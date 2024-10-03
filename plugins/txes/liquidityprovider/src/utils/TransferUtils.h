/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "src/state/LiquidityProviderEntry.h"
#include <optional>

namespace catapult::utils {

	using BigUint = boost::multiprecision::uint256_t;

	// Optional is returned in order to handle overflows
	std::optional<Amount> computeCreditCurrencyAmount(const state::LiquidityProviderEntry& lpEntry,
									   const Amount& currencyBalance,
									   const Amount& mosaicBalance,
									   const Amount& mosaicAmount,
									   uint8_t percentsDigitAfterDot);

	// Overflow is not possible in case of debit mosaics
	Amount computeDebitCurrencyAmount(const state::LiquidityProviderEntry& lpEntry,
									  const Amount& currencyBalance,
									  const Amount& mosaicBalance,
									  const Amount& mosaicAmount,
									  uint8_t percentsDigitAfterDot);

	bool isValidAmount(const BigUint& value);

} // namespace catapult::utils