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

#include "TransferUtils.h"
#include "MathUtils.h"
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult::utils {

	using BigUint = boost::multiprecision::uint256_t;

	Amount computeCreditCurrencyAmount(
			const state::LiquidityProviderEntry& lpEntry,
			const Amount& currencyBalance,
			const Amount& mosaicBalance,
			const Amount& mosaicAmount,
			uint8_t percentsDigitAfterDot) {
		auto rate = state::ExchangeRate{currencyBalance, mosaicBalance + lpEntry.additionallyMinted()};
		BigUint numerator = BigUint(mosaicAmount.unwrap()) * rate.m_currencyAmount.unwrap() * (100 * pow(10, percentsDigitAfterDot) + lpEntry.alpha());
		BigUint denominator = BigUint(100) * rate.m_mosaicAmount.unwrap();

		// In order to avoid the problems with rounding, LP receives a little more (due to ceil)
		return Amount{ceilDivision(numerator, denominator).convert_to<uint64_t>()};
	}

	Amount computeDebitCurrencyAmount(const state::LiquidityProviderEntry& lpEntry,
									  const Amount& currencyBalance,
									  const Amount& mosaicBalance,
									  const Amount& mosaicAmount,
									  uint8_t percentsDigitAfterDot) {
		auto rate = state::ExchangeRate{currencyBalance, mosaicBalance + lpEntry.additionallyMinted()};
		BigUint numerator = BigUint(mosaicAmount.unwrap()) * rate.m_currencyAmount.unwrap() * (100 * pow(10, percentsDigitAfterDot) - lpEntry.alpha());
		BigUint denominator = BigUint(100) * rate.m_mosaicAmount.unwrap();

		// In order to avoid the problems with rounding, LP looses a little less (due to floor)
		return Amount{floorDivision(numerator, denominator).convert_to<uint64_t>()};
	}
}