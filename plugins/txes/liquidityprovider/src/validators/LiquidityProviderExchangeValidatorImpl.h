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
#include "src/catapult/validators/LiquidityProviderExchangeValidator.h"

namespace catapult { namespace validators {

	class LiquidityProviderExchangeValidatorImpl : public LiquidityProviderExchangeValidator {
	public:
		ValidationResult validateCreditMosaics(
				const ValidatorContext& context,
				const Key& currencyDebtor,
				const UnresolvedMosaicId& unresolvedMosaicId,
				const UnresolvedAmount& mosaicAmount) const override;

		ValidationResult validateDebitMosaics(
				const ValidatorContext& context,
				const Key& mosaicCreditor,
				const UnresolvedMosaicId& unresolvedMosaicId,
				const UnresolvedAmount& mosaicAmount) const override;

		ValidationResult validateCreditMosaics(
				const ValidatorContext& context,
				const Key& currencyDebtor,
				const UnresolvedMosaicId& mosaicId,
				const Amount& mosaicAmount) const override;
		ValidationResult validateDebitMosaics(
				const ValidatorContext& context,
				const Key& mosaicCreditor,
				const UnresolvedMosaicId& mosaicId,
				const Amount& mosaicAmount) const override;
	};
}}