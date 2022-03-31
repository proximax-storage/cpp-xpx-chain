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
		const auto& liquidityProviderCache = context.Cache.sub<cache::LiquidityProviderCache>();

		const auto& entry = liquidityProviderCache.find(notification.ProviderMosaicId).get();

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto& lpStateEntry = accountStateCache.find(entry.providerKey()).get();
		auto& ownerStateEntry = accountStateCache.find(entry.owner()).get();
		auto& slashingStateEntry = accountStateCache.find(entry.slashingAccount()).get();

		const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
		if (notification.CurrencyBalanceIncrease) {
			ownerStateEntry.Balances.debit(currencyMosaicId, notification.CurrencyBalanceChange);
			lpStateEntry.Balances.credit(currencyMosaicId, notification.CurrencyBalanceChange);
		}
		else {
			lpStateEntry.Balances.debit(currencyMosaicId, notification.CurrencyBalanceChange);
			slashingStateEntry.Balances.credit(currencyMosaicId, notification.CurrencyBalanceChange);
		}


		// We can not allow zero on LP balance
		auto resolvedMosaicId = context.Resolvers.resolve(entry.mosaicId());
		if (notification.MosaicBalanceIncrease) {
			lpStateEntry.Balances.credit(resolvedMosaicId, notification.MosaicBalanceChange);
		}
		else {
			lpStateEntry.Balances.debit(resolvedMosaicId, notification.MosaicBalanceChange);
		}
	}));
}}
