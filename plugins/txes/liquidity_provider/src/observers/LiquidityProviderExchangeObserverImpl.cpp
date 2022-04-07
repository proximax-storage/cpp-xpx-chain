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
#include <catapult/cache_core/AccountStateCache.h>
#include "src/utils/TransferUtils.h"
#include "src/cache/LiquidityProviderCache.h"
#include "LiquidityProviderExchangeObserverImpl.h"

namespace catapult::observers {

	void LiquidityProviderExchangeObserverImpl::creditMosaics(
			ObserverContext& context,
			const Key& currencyDebtor,
			const Key& mosaicCreditor,
			const UnresolvedMosaicId& unresolvedMosaicId,
			const UnresolvedAmount& unresolvedMosaicAmount) const {
		auto resolvedAmount = context.Resolvers.resolve(unresolvedMosaicAmount);
		creditMosaics(context, currencyDebtor, mosaicCreditor, unresolvedMosaicId, resolvedAmount);
	}

	void LiquidityProviderExchangeObserverImpl::debitMosaics(
			ObserverContext& context,
			const Key& mosaicDebtor,
			const Key& currencyCreditor,
			const UnresolvedMosaicId& unresolvedMosaicId,
			const UnresolvedAmount& unresolvedMosaicAmount) const {
		auto resolvedAmount = context.Resolvers.resolve(unresolvedMosaicAmount);
		debitMosaics(context, mosaicDebtor, currencyCreditor, unresolvedMosaicId, resolvedAmount);
	}

	void LiquidityProviderExchangeObserverImpl::creditMosaics(
			ObserverContext& context,
			const Key& currencyDebtor,
			const Key& mosaicCreditor,
			const UnresolvedMosaicId& mosaicId,
			const Amount& mosaicAmount) const {
		auto& lpCache = context.Cache.sub<cache::LiquidityProviderCache>();

		auto& lpEntry = lpCache.find(mosaicId).get();

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();

		auto& lpAccount = accountStateCache.find(lpEntry.providerKey()).get();

		const auto& pluginConfig =
				context.Config.Network.template GetPluginConfiguration<config::LiquidityProviderConfiguration>();

		const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;

		auto resolvedMosaicId = context.Resolvers.resolve(mosaicId);

		// In the observer the optional alwasys has the value
		auto currencyAmount = *utils::computeCreditCurrencyAmount(
				lpEntry,
				lpAccount.Balances.get(currencyMosaicId),
				lpAccount.Balances.get(resolvedMosaicId),
				mosaicAmount,
				pluginConfig.PercentsDigitsAfterDot);

		auto& debtorAccount = accountStateCache.find(currencyDebtor).get();
		debtorAccount.Balances.debit(currencyMosaicId, currencyAmount);

		lpAccount.Balances.credit(currencyMosaicId, currencyAmount);

		auto& creditorAccount = accountStateCache.find(mosaicCreditor).get();
		creditorAccount.Balances.credit(resolvedMosaicId, mosaicAmount);

		lpEntry.setAdditionallyMinted(lpEntry.additionallyMinted() + mosaicAmount);

		lpEntry.recentTurnover().m_turnover = lpEntry.recentTurnover().m_turnover + currencyAmount;
	}

	void LiquidityProviderExchangeObserverImpl::debitMosaics(
			ObserverContext& context,
			const Key& mosaicDebtor,
			const Key& currencyCreditor,
			const UnresolvedMosaicId& mosaicId,
			const Amount& mosaicAmount) const {
		auto& lpCache = context.Cache.sub<cache::LiquidityProviderCache>();

		auto& lpEntry = lpCache.find(mosaicId).get();

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto& lpAccount = accountStateCache.find(lpEntry.providerKey()).get();

		const auto& pluginConfig =
				context.Config.Network.template GetPluginConfiguration<config::LiquidityProviderConfiguration>();

		const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;

		auto resolvedMosaicId = context.Resolvers.resolve(mosaicId);
		Amount currencyAmount = utils::computeDebitCurrencyAmount(
				lpEntry,
				lpAccount.Balances.get(currencyMosaicId),
				lpAccount.Balances.get(resolvedMosaicId),
				mosaicAmount,
				pluginConfig.PercentsDigitsAfterDot);

		auto& creditorAccount = accountStateCache.find(currencyCreditor).get();
		creditorAccount.Balances.credit(currencyMosaicId, currencyAmount);

		auto a = lpAccount.Balances.get(currencyMosaicId);
		lpAccount.Balances.debit(currencyMosaicId, currencyAmount);
		auto b = lpAccount.Balances.get(currencyMosaicId);

		auto& debtorAccount = accountStateCache.find(mosaicDebtor).get();
		debtorAccount.Balances.debit(resolvedMosaicId, mosaicAmount);

		lpEntry.setAdditionallyMinted(lpEntry.additionallyMinted() - mosaicAmount);

		lpEntry.recentTurnover().m_turnover = lpEntry.recentTurnover().m_turnover + currencyAmount;
	}
} // namespace catapult::observers