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
			const Key& debtor,
			const UnresolvedMosaicId& mosaicId,
			const Amount& mosaicAmount,
			const MosaicId& currencyId) const {
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

		Amount currencyAmount = utils::computeCreditCurrencyAmount(
				*pLpEntry,
				lpAccount.Balances.get(currencyId),
				lpAccount.Balances.get(resolvedMosaicId),
				mosaicAmount,
				pluginConfig.PercentsDigitsAfterDot);

		const auto* pDebtorAccount = accountStateCache.find(debtor).tryGet();

		// Unlikely
		if (!pDebtorAccount) {
			return Failure_LiquidityProvider_Insufficient_Currency;
		}

		auto currencyBalance = pDebtorAccount->Balances.get(currencyId);

		if (currencyBalance < currencyAmount) {
			return Failure_LiquidityProvider_Insufficient_Currency;
		}

		return ValidationResult::Success;
	}

	ValidationResult LiquidityProviderExchangeValidatorImpl::validateDebitMosaics(
			const ValidatorContext& context,
			const Key& creditor,
			const UnresolvedMosaicId& mosaicId,
			const Amount& mosaicAmount) const {

		const auto& lpCache = context.Cache.sub<cache::LiquidityProviderCache>();

		const auto* pLpEntry = lpCache.find(mosaicId).tryGet();

		if (!pLpEntry) {
			return Failure_LiquidityProvider_Liquidity_Provider_Is_Not_Registered;
		}

		const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		const auto* pDebtorAccount = accountStateCache.find(creditor).tryGet();

		// Unlikely
		if (!pDebtorAccount) {
			return Failure_LiquidityProvider_Insufficient_Currency;
		}

		auto resolvedMosaicId = context.Resolvers.resolve(mosaicId);
		auto mosaicBalance = pDebtorAccount->Balances.get(resolvedMosaicId);

		if (mosaicBalance < mosaicAmount) {
			return Failure_LiquidityProvider_Insufficient_Currency;
		}

		return ValidationResult::Success;
	}
}
