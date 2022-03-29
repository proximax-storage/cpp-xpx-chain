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
			const Key& debtor,
			const UnresolvedMosaicId& mosaicId,
			const Amount& mosaicAmount,
			const MosaicId& currencyId) const {
		auto& lpCache = context.Cache.sub<cache::LiquidityProviderCache>();

		auto& lpEntry = lpCache.find(mosaicId).get();

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto& debtorAccount = accountStateCache.find(debtor).get();
		auto& lpAccount = accountStateCache.find(lpEntry.providerKey()).get();

		const auto& pluginConfig =
				context.Config.Network.template GetPluginConfiguration<config::LiquidityProviderConfiguration>();

		auto resolvedMosaicId = context.Resolvers.resolve(mosaicId);
		Amount currencyAmount = utils::computeCreditCurrencyAmount(
				lpEntry,
				lpAccount.Balances.get(currencyId),
				lpAccount.Balances.get(resolvedMosaicId),
				mosaicAmount,
				pluginConfig.PercentsDigitsAfterDot);

		debtorAccount.Balances.debit(currencyId, currencyAmount);
		lpAccount.Balances.credit(currencyId, currencyAmount);

		debtorAccount.Balances.credit(resolvedMosaicId, mosaicAmount);

		lpEntry.setAdditionallyMinted(lpEntry.additionallyMinted() + mosaicAmount);

		lpEntry.recentTurnover().m_turnover = lpEntry.recentTurnover().m_turnover + currencyAmount;
	}

	void LiquidityProviderExchangeObserverImpl::debitMosaics(
			ObserverContext& context,
			const Key& creditor,
			const UnresolvedMosaicId& mosaicId,
			const Amount& mosaicAmount,
			const MosaicId& currencyId) const {
		auto& lpCache = context.Cache.sub<cache::LiquidityProviderCache>();

		auto& lpEntry = lpCache.find(mosaicId).get();

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto& lpAccount = accountStateCache.find(lpEntry.providerKey()).get();

		const auto& pluginConfig =
				context.Config.Network.template GetPluginConfiguration<config::LiquidityProviderConfiguration>();

		auto resolvedMosaicId = context.Resolvers.resolve(mosaicId);
		Amount currencyAmount = utils::computeDebitCurrencyAmount(
				lpEntry,
				lpAccount.Balances.get(currencyId),
				lpAccount.Balances.get(resolvedMosaicId),
				mosaicAmount,
				pluginConfig.PercentsDigitsAfterDot);

		auto& creditorAccount = accountStateCache.find(creditor).get();

		creditorAccount.Balances.credit(currencyId, currencyAmount);
		lpAccount.Balances.debit(currencyId, currencyAmount);

		creditorAccount.Balances.debit(resolvedMosaicId, mosaicAmount);

		lpEntry.setAdditionallyMinted(lpEntry.additionallyMinted() - mosaicAmount);

		lpEntry.recentTurnover().m_turnover = lpEntry.recentTurnover().m_turnover + currencyAmount;
	}
} // namespace catapult::observers