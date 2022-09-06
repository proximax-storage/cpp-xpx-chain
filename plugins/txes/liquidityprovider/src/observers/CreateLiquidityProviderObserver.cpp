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

	using Notification = model::CreateLiquidityProviderNotification<1>;

	DEFINE_OBSERVER(CreateLiquidityProvider, Notification, ([](const Notification& notification, ObserverContext& context) {
	  	if (NotifyMode::Rollback == context.Mode)
		  	CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (CreateLiquidityProvider)");

		state::LiquidityProviderEntry entry(notification.ProviderMosaicId);
		entry.setProviderKey(notification.ProviderKey);
		entry.setOwner(notification.Owner);
		entry.setSlashingAccount(notification.SlashingAccount);
		entry.setCreationHeight(context.Height);
		entry.setWindowSize(notification.WindowSize);
		entry.setSlashingPeriod(notification.SlashingPeriod);
		entry.setAlpha(notification.Alpha);
		entry.setBeta(notification.Beta);

		entry.recentTurnover() = state::HistoryObservation{ {notification.CurrencyDeposit, notification.InitialMosaicsMinting}, Amount{0} };

		auto& liquidityProviderCache = context.Cache.sub<cache::LiquidityProviderCache>();
		liquidityProviderCache.insert(entry);
	}));
}}
