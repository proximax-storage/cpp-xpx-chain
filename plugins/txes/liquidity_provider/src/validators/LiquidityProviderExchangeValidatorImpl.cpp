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

#include <catapult/model/ResolverContext.h>
#include "src/catapult/cache_core/AccountStateCache.h"
#include "LiquidityProviderExchangeValidatorImpl.h"
#include "src/catapult/cache/ReadOnlyCatapultCache.h"
#include "src/cache/LiquidityProviderCache.h"
#include "Results.h"
#include "src/utils/TransferUtils.h"

namespace catapult::validators {

	ValidationResult LiquidityProviderExchangeValidatorImpl::validateCreditMosaics(
			const ValidatorContext& context,
			const Key& currencyDebtor,
			const UnresolvedMosaicId& unresolvedMosaicId,
			const UnresolvedAmount& unresolvedMosaicAmount) const {
		auto resolvedAmount = context.Resolvers.resolve(unresolvedMosaicAmount);
		return validateCreditMosaics(context, currencyDebtor, unresolvedMosaicId, resolvedAmount);
	}

	ValidationResult LiquidityProviderExchangeValidatorImpl::validateDebitMosaics(
			const ValidatorContext& context,
			const Key& mosaicCreditor,
			const UnresolvedMosaicId& unresolvedMosaicId,
			const UnresolvedAmount& unresolvedMosaicAmount) const {
		auto resolvedAmount = context.Resolvers.resolve(unresolvedMosaicAmount);
		return validateDebitMosaics(context, mosaicCreditor, unresolvedMosaicId, resolvedAmount);
	}

	ValidationResult LiquidityProviderExchangeValidatorImpl::validateCreditMosaics(
			const ValidatorContext& context,
			const Key& currencyDebtor,
			const UnresolvedMosaicId& mosaicId,
			const Amount& mosaicAmount) const {
		const auto& lpCache = context.Cache.sub<cache::LiquidityProviderCache>();

		const auto* pLpEntry = lpCache.find(mosaicId).tryGet();

		if (!pLpEntry) {
			return Failure_LiquidityProvider_Liquidity_Provider_Is_Not_Registered;
		}

		const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto& lpAccount = accountStateCache.find(pLpEntry->providerKey()).get();

		const auto& pluginConfig =
				context.Config.Network.template GetPluginConfiguration<config::LiquidityProviderConfiguration>();

		auto resolvedMosaicId = context.Resolvers.resolve(mosaicId);

		const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;

		auto expectedMosaicMinted = utils::BigUint(lpAccount.Balances.get(resolvedMosaicId).unwrap()) +
									utils::BigUint(pLpEntry->additionallyMinted().unwrap()) +
									utils::BigUint(mosaicAmount.unwrap());

		if (!utils::isValidAmount(expectedMosaicMinted)) {
			return Failure_LiquidityProvider_Invalid_Mosaic_Amount;
		}

		auto optionalCurrencyAmount = utils::computeCreditCurrencyAmount(
				*pLpEntry,
				lpAccount.Balances.get(currencyMosaicId),
				lpAccount.Balances.get(resolvedMosaicId),
				mosaicAmount,
				pluginConfig.PercentsDigitsAfterDot);

		if (!optionalCurrencyAmount) {
			// An attempt to emit to many mosaics
			return Failure_LiquidityProvider_Invalid_Currency_Amount;
		}

		auto currencyAmount = *optionalCurrencyAmount;

		const auto* pDebtorAccount = accountStateCache.find(currencyDebtor).tryGet();

		// Unlikely
		if (!pDebtorAccount) {
			return Failure_LiquidityProvider_Insufficient_Currency;
		}

		auto currencyBalance = pDebtorAccount->Balances.get(currencyMosaicId);

		if (currencyBalance < currencyAmount) {
			return Failure_LiquidityProvider_Insufficient_Currency;
		}

		return ValidationResult::Success;
	}

	ValidationResult LiquidityProviderExchangeValidatorImpl::validateDebitMosaics(
			const ValidatorContext& context,
			const Key& mosaicCreditor,
			const UnresolvedMosaicId& mosaicId,
			const Amount& mosaicAmount) const {
		const auto& lpCache = context.Cache.sub<cache::LiquidityProviderCache>();

		const auto* pLpEntry = lpCache.find(mosaicId).tryGet();

		if (!pLpEntry) {
			return Failure_LiquidityProvider_Liquidity_Provider_Is_Not_Registered;
		}

		const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		const auto* pDebtorAccount = accountStateCache.find(mosaicCreditor).tryGet();

		// Unlikely
		if (!pDebtorAccount) {
			return Failure_LiquidityProvider_Insufficient_Mosaic;
		}

		auto resolvedMosaicId = context.Resolvers.resolve(mosaicId);
		auto mosaicBalance = pDebtorAccount->Balances.get(resolvedMosaicId);

		if (mosaicBalance < mosaicAmount) {
			return Failure_LiquidityProvider_Insufficient_Mosaic;
		}

		return ValidationResult::Success;
	}
}
