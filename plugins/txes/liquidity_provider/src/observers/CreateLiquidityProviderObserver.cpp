/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"
#include "src/model/InternalLiquidityProviderNotifications.h"
#include "src/state/LPEntry.h"

namespace catapult { namespace observers {

	using Notification = model::CreateLiquidityProviderNotification<1>;

	DEFINE_OBSERVER(CreateLiquidityProvider, Notification, ([](const Notification& notification, ObserverContext& context) {
		state::LiquidityProviderEntry entry(notification.ProviderMosaicId);
		entry.setProviderKey(notification.ProviderKey);
		entry.setOwner(notification.Owner);
		entry.setSlashingAccount(notification.SlashingAccount);
		entry.setCreationHeight(context.Height);
		entry.setWindowSize(notification.WindowSize);
		entry.setSlashingPeriod(notification.SlashingPeriod);
		entry.setAlpha(notification.Alpha);
		entry.setBeta(notification.Beta);

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto& accountEntry = accountStateCache.find(notification.ProviderKey).get();

		const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
		auto currencyBalance = accountEntry.Balances.get(currencyMosaicId);

		auto resolvedMosaicId = context.Resolvers.resolve(entry.mosaicId());
		auto mosaicBalance = accountEntry.Balances.get(resolvedMosaicId);

		entry.recentTurnover() = state::HistoryObservation{ {currencyBalance, mosaicBalance}, Amount{0} };
	}));
}}
