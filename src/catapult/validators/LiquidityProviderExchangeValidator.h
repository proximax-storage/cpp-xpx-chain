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
#include "ValidationResult.h"
#include "ValidatorContext.h"

namespace catapult { namespace validators {

	class LiquidityProviderExchangeValidator {
	public:

		virtual ~LiquidityProviderExchangeValidator() = default;

		// Verifies whether the debtor has enough CURRENCY to buy the approppriate amount of Mosaics
		virtual ValidationResult validateCreditMosaics(const ValidatorContext& context,
													   const Key& debtor,
													   const UnresolvedMosaicId& mosaicId,
													   const Amount& mosaicAmount,
													   const MosaicId& currencyId) const = 0;

		// Verifies whether the creditor has enough MOSAIC to sell
		virtual ValidationResult validateDebitMosaics(const ValidatorContext& context,
													  const Key& creditor,
													  const UnresolvedMosaicId& mosaicId,
													  const Amount& mosaicAmount) const = 0;
	};
}}