/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(SnapshotCleanUp, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(SnapshotCleanUp, Notification, [pConfigHolder](const auto&, const ObserverContext& context) {
			if (context.Mode == NotifyMode::Rollback)
				return;

			const model::NetworkConfiguration& config = pConfigHolder->Config(context.Height).Network;
			if (config.MaxRollbackBlocks != 0 && context.Height.unwrap() % config.MaxRollbackBlocks)
				return;

			auto& cache = context.Cache.sub<cache::AccountStateCache>();
			auto updatedAddresses = cache.updatedAddresses();

			for (const auto& address : updatedAddresses) {
				auto pAccountState = cache.find(address).tryGet();

				if (!pAccountState)
					continue;

				pAccountState->Balances.maybeCleanUpSnapshots(context.Height, config);
			}
		});
	}
}}
