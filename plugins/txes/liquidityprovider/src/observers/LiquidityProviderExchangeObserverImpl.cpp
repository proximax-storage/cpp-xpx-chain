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

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto balanceBefore = accountStateCache.find(currencyDebtor).get().Balances.get(context.Config.Immutable.CurrencyMosaicId);

		CATAPULT_LOG(error) << "Credit Mosaics Before " << balanceBefore;

		auto& lpCache = context.Cache.sub<cache::LiquidityProviderCache>();

		auto lpEntryIter = lpCache.find(mosaicId);
		auto& lpEntry = lpEntryIter.get();



		auto lpAccountIter = accountStateCache.find(lpEntry.providerKey());
		auto& lpAccount = lpAccountIter.get();

		const auto& pluginConfig =
				context.Config.Network.template GetPluginConfiguration<config::LiquidityProviderConfiguration>();

		const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;

		auto resolvedMosaicId = context.Resolvers.resolve(mosaicId);

		// In the observer the optional always has the value
		auto currencyAmount = *utils::computeCreditCurrencyAmount(
				lpEntry,
				lpAccount.Balances.get(currencyMosaicId),
				lpAccount.Balances.get(resolvedMosaicId),
				mosaicAmount,
				pluginConfig.PercentsDigitsAfterDot);

		auto debtorAccountIter = accountStateCache.find(currencyDebtor);
		auto& debtorAccount = debtorAccountIter.get();

		if ( debtorAccount.Balances.get(currencyMosaicId) < currencyAmount ) {
			CATAPULT_LOG( error ) << "Debtor Not Enough Currency "
				<< debtorAccount.Balances.get(currencyMosaicId) << " "
				<< currencyAmount;
			return;
		}

		debtorAccount.Balances.debit(currencyMosaicId, currencyAmount);

		lpAccount.Balances.credit(currencyMosaicId, currencyAmount);

		auto creditorAccountIter = accountStateCache.find(mosaicCreditor);
		auto& creditorAccount = creditorAccountIter.get();
		creditorAccount.Balances.credit(resolvedMosaicId, mosaicAmount);

		lpEntry.setAdditionallyMinted(lpEntry.additionallyMinted() + mosaicAmount);

		lpEntry.recentTurnover().m_turnover = lpEntry.recentTurnover().m_turnover + currencyAmount;

		auto balanceAfter = accountStateCache.find(currencyDebtor).get().Balances.get(context.Config.Immutable.CurrencyMosaicId);

		CATAPULT_LOG(error) << "Credit Mosaics After " << balanceAfter;
	}

	void LiquidityProviderExchangeObserverImpl::debitMosaics(
			ObserverContext& context,
			const Key& mosaicDebtor,
			const Key& currencyCreditor,
			const UnresolvedMosaicId& mosaicId,
			const Amount& mosaicAmount) const {
		auto& lpCache = context.Cache.sub<cache::LiquidityProviderCache>();

		auto lpEntryIter = lpCache.find(mosaicId);
		auto& lpEntry = lpEntryIter.get();

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto lpAccountIter = accountStateCache.find(lpEntry.providerKey());
		auto& lpAccount = lpAccountIter.get();

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

		if ( lpAccount.Balances.get(currencyMosaicId) < currencyAmount ) {
			CATAPULT_LOG( error ) << "LP Not Enough Currency "
								<< lpAccount.Balances.get(currencyMosaicId) << " "
								<< currencyAmount;
			return;
		}

		auto debtorAccountIter = accountStateCache.find(mosaicDebtor);
		auto& debtorAccount = debtorAccountIter.get();

		if ( debtorAccount.Balances.get(resolvedMosaicId) < mosaicAmount ) {
			CATAPULT_LOG( error ) << "Debtor Not Enough Mosaics "
								<< mosaicDebtor << " "
								<< resolvedMosaicId << ""
								<< debtorAccount.Balances.get(resolvedMosaicId) << " "
								<< mosaicAmount;
			return;
		}

		auto creditorAccountIter = accountStateCache.find(currencyCreditor);
		auto& creditorAccount = creditorAccountIter.get();
		creditorAccount.Balances.credit(currencyMosaicId, currencyAmount);

		lpAccount.Balances.debit(currencyMosaicId, currencyAmount);

		debtorAccount.Balances.debit(resolvedMosaicId, mosaicAmount);

		lpEntry.setAdditionallyMinted(lpEntry.additionallyMinted() - mosaicAmount);

		lpEntry.recentTurnover().m_turnover = lpEntry.recentTurnover().m_turnover + currencyAmount;
	}
} // namespace catapult::observers