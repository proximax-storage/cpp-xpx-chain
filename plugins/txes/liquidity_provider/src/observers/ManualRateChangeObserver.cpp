/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include <src/cache/LiquidityProviderCache.h>
#include "Observers.h"
#include "src/model/InternalLiquidityProviderNotifications.h"
#include "src/state/LiquidityProviderEntry.h"

namespace catapult { namespace observers {

	using Notification = model::ManualRateChangeNotification<1>;

	DEFINE_OBSERVER(ManualRateChange, Notification, ([](const Notification& notification, ObserverContext& context) {
		auto& liquidityProviderCache = context.Cache.sub<cache::LiquidityProviderCache>();

		auto entryIter = liquidityProviderCache.find(notification.ProviderMosaicId);
		auto& entry = entryIter.get();

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();

		auto lpStateEntryIter = accountStateCache.find(entry.providerKey());
		auto& lpStateEntry = lpStateEntryIter.get();

		auto ownerStateEntryIter = accountStateCache.find(entry.owner());
		auto& ownerStateEntry = ownerStateEntryIter.get();

		auto slashingStateEntryIter = accountStateCache.find(entry.slashingAccount());
		auto& slashingStateEntry = slashingStateEntryIter.get();

		const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
		if (notification.CurrencyBalanceIncrease) {
			ownerStateEntry.Balances.debit(currencyMosaicId, notification.CurrencyBalanceChange);
			lpStateEntry.Balances.credit(currencyMosaicId, notification.CurrencyBalanceChange);
		} else {
			lpStateEntry.Balances.debit(currencyMosaicId, notification.CurrencyBalanceChange);
			slashingStateEntry.Balances.credit(currencyMosaicId, notification.CurrencyBalanceChange);
		}

		auto resolvedMosaicId = context.Resolvers.resolve(entry.mosaicId());
		if (notification.MosaicBalanceIncrease) {
			lpStateEntry.Balances.credit(resolvedMosaicId, notification.MosaicBalanceChange);
		} else {
			lpStateEntry.Balances.debit(resolvedMosaicId, notification.MosaicBalanceChange);
		}

		entry.setCreationHeight(context.Height);
		entry.turnoverHistory().clear();
		entry.recentTurnover() =
				state::HistoryObservation { { lpStateEntry.Balances.get(currencyMosaicId),
											  lpStateEntry.Balances.get(resolvedMosaicId) },
											Amount { 0 } };
	}));
}} // namespace catapult::observers
