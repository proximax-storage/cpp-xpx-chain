/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DbrbViewCache.h"
#include "src/cache/DbrbViewFetcherImpl.h"
#include "catapult/chain/CommitteeManager.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DbrbProcessPruning, model::BlockNotification<1>, [](const auto& notification, const ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			return;

		auto maxTransactionLifeTime = Timestamp(context.Config.Network.MaxTransactionLifetime.millis());
		auto blockTargetTime = chain::CommitteePhaseCount * context.Config.Network.MinCommitteePhaseTime.millis();
		uint64_t blockInterval = maxTransactionLifeTime.unwrap() / blockTargetTime;
		if (context.Height.unwrap() % blockInterval != 0 || blockInterval < context.Height.unwrap() || context.Timestamp < maxTransactionLifeTime)
			return;

		auto& cache = context.Cache.sub<cache::DbrbViewCache>();
		auto processIds = cache.processIds();
		auto pruningBoundary = context.Timestamp - maxTransactionLifeTime;
		for (const auto& processId : processIds) {
			auto iter = cache.find(processId);
			const auto& entry = iter.get();
			if (entry.expirationTime() < pruningBoundary) {
				CATAPULT_LOG(debug) << "removing DBRB process " << processId;
				cache.remove(processId);
			}
		}
	});
}}
