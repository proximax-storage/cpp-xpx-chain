/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CommitteeCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/ImportanceView.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(AddHarvester, model::AddHarvesterNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::CommitteeCache>();
		if (NotifyMode::Commit == context.Mode) {
			auto readOnlyCache = context.Cache.toReadOnly();
			cache::ImportanceView view(readOnlyCache.sub<cache::AccountStateCache>());
			auto effectiveBalance = view.getAccountImportanceOrDefault(notification.HarvesterKey, context.Height);
			const auto& config = context.Config.Network.GetPluginConfiguration<config::CommitteeConfiguration>();
			cache.insert(state::CommitteeEntry(notification.HarvesterKey, notification.Signer, context.Height, effectiveBalance, true, config.InitialActivity, config.MinGreed));
		} else {
			cache.remove(notification.HarvesterKey);
		}
	});
}}
