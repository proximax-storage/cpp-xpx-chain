/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DbrbViewCache.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/observers/DbrbProcessUpdateListener.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	namespace {
		constexpr VersionType Block_Version = 7;
	}

	DECLARE_OBSERVER(DbrbProcessUpdate, Notification)(const plugins::PluginManager& pluginManager) {
		return MAKE_OBSERVER(DbrbProcessUpdate, Notification, ([&pluginManager](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DbrbProcessUpdate)");

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::DbrbConfiguration>();
			if (!pluginConfig.EnableDbrbProcessBanning)
				return;

			const auto& committeeManager = pluginManager.getCommitteeManager(Block_Version);
			auto banPeriods = committeeManager.banPeriods();
			auto& cache = context.Cache.sub<cache::DbrbViewCache>();
			auto processIds = cache.processIds();
			for (const auto& processId : processIds) {
				auto dbrbProcessIter = cache.find(processId);
				auto& entry = dbrbProcessIter.get();
				entry.setVersion(2);
				auto banPeriodIter = banPeriods.find(processId);
				if (banPeriodIter != banPeriods.cend())
					entry.setBanPeriod(banPeriodIter->second);
				entry.decrementBanPeriod();
			}
        }))
	};
}}
