/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DbrbViewCache.h"
#include "catapult/chain/CommitteeManager.h"
#include "catapult/observers/DbrbProcessUpdateListener.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(DbrbProcessPruning, Notification)(const std::vector<std::unique_ptr<DbrbProcessUpdateListener>>& updateListeners) {
		return MAKE_OBSERVER(DbrbProcessPruning, Notification, ([&updateListeners](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DbrbProcessPruning)");

			auto maxTransactionLifeTime = Timestamp(context.Config.Network.MaxTransactionLifetime.millis());
			auto blockTargetTime = chain::CommitteePhaseCount * context.Config.Network.MinCommitteePhaseTime.millis();
			uint64_t blockInterval = maxTransactionLifeTime.unwrap() / blockTargetTime;
			if (context.Height.unwrap() % blockInterval != 0 || blockInterval < context.Height.unwrap() || context.Timestamp < maxTransactionLifeTime)
				return;

			auto& cache = context.Cache.sub<cache::DbrbViewCache>();
			auto processIds = cache.processIds();
			const auto& config = context.Config.Network.GetPluginConfiguration<config::DbrbConfiguration>();
			auto lifetime = config.DbrbProcessLifetimeAfterExpiration.millis();
			auto pruningBoundary = (lifetime > 0 ? context.Timestamp - Timestamp(lifetime) : context.Timestamp - maxTransactionLifeTime);
			for (const auto& processId : processIds) {
				auto iter = cache.find(processId);
				const auto& entry = iter.get();
				if (entry.expirationTime() < pruningBoundary) {
					cache.remove(processId);
					for (const auto& pListener : updateListeners)
						pListener->OnDbrbProcessRemoved(context, processId);
				}
			}
        }))
	};
}}
