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

	bool isValidAmount(const BigUint& value) {
		auto truncated = value.convert_to<uint64_t>();
		if (value == truncated) {
			return true;
		}
		return false;
	}

	std::optional<Amount> computeCreditCurrencyAmount(
			const state::LiquidityProviderEntry& lpEntry,
			const Amount& currencyBalance,
			const Amount& mosaicBalance,
			const Amount& mosaicAmount,
			uint8_t percentsDigitAfterDot) {
		auto rate = state::ExchangeRate{currencyBalance, mosaicBalance + lpEntry.additionallyMinted()};
		BigUint numerator = (BigUint(mosaicAmount.unwrap()) * rate.m_currencyAmount.unwrap()) * (static_cast<uint64_t>(100) * pow(10, percentsDigitAfterDot) + lpEntry.alpha());
		BigUint denominator = BigUint(static_cast<uint64_t>(100) * pow(10, percentsDigitAfterDot)) * rate.m_mosaicAmount.unwrap();

		// In order to avoid the problems with rounding, LP receives a little more (due to ceil)
		BigUint amount = ceilDivision(numerator, denominator);

		if (!isValidAmount(amount)) {
			return {};
		}

		return Amount{amount.convert_to<uint64_t>()};
	}

	Amount computeDebitCurrencyAmount(const state::LiquidityProviderEntry& lpEntry,
									  const Amount& currencyBalance,
									  const Amount& mosaicBalance,
									  const Amount& mosaicAmount,
									  uint8_t percentsDigitAfterDot) {
		auto rate = state::ExchangeRate{currencyBalance, mosaicBalance + lpEntry.additionallyMinted()};
		BigUint numerator = (BigUint(mosaicAmount.unwrap()) * rate.m_currencyAmount.unwrap()) * (static_cast<uint64_t>(100) * pow(10, percentsDigitAfterDot) - lpEntry.beta());
		BigUint denominator = BigUint(static_cast<uint64_t>(100) * pow(10, percentsDigitAfterDot)) * rate.m_mosaicAmount.unwrap();

		// In order to avoid the problems with rounding, LP looses a little less (due to floor)
		BigUint amount = floorDivision(numerator, denominator);

		if (!isValidAmount(amount))
		{
			CATAPULT_LOG(error) << "Invalid Amount in Debit Mosaics " << currencyBalance << " " << mosaicBalance << " "
								<< lpEntry.additionallyMinted() << " " << mosaicAmount;
		}

		return Amount{amount.convert_to<uint64_t>()};
	}
}