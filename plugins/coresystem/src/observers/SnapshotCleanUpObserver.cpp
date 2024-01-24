/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "plugins/txes/config/src/cache/NetworkConfigCache.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;
	namespace {
		auto ObserveNotification(const std::shared_ptr<config::BlockchainConfigurationHolder>& holder) {
			return [&holder](const auto&, const ObserverContext& context) {
				if (context.Mode == NotifyMode::Rollback)
					return;

				const model::NetworkConfiguration& config = context.Config.Network;
				if (config.MaxRollbackBlocks != 0 && context.Height.unwrap() % config.MaxRollbackBlocks)
					return;



				/// When using proper effective balance calculation, we must check which configuration we are going to be using.
				/// We will only use the new network configuration ImportanceGrouping + MaxRollBackBlocks value if we are past the the height at which the networkconfiguration would be inside the stable height.
				/// This would mean that a breaking rollback could only happen if the chain was rolled at least two times the MaxRollbackHeight which is very unlikely.

				auto& configCache = context.Cache.sub<cache::NetworkConfigCache>();
				auto IGMRB = state::StableBalanceSnapshot(config.ImportanceGrouping, config.MaxRollbackBlocks);
				if(config.ProperEffectiveBalanceCalculation)
				{
					auto existingConfigurations = configCache.heights();
					auto currentConfigIter = existingConfigurations.find(context.Height);
					auto rCurrentConfigIter = ++std::make_reverse_iterator(currentConfigIter);

					for (auto rit = rCurrentConfigIter; rit != existingConfigurations.rend(); ++rit) {
						auto& previousConfig = holder->Config(*rit).Network;
						auto nIGMRB = state::StableBalanceSnapshot(previousConfig.ImportanceGrouping, previousConfig.MaxRollbackBlocks);
						if(IGMRB != nIGMRB) {
							/// We must verify whether this config still has validity or if it's past the acceptable range.
							if(context.Height - nIGMRB.GetUnstableHeight(2) < *rit) {
								IGMRB = nIGMRB;
							}
							break;
						}
					}
				}

				/// At this point we are aware of the stable height calculation value we are using, we can pass that to clean up snapshots evaluator function.

				auto& cache = context.Cache.sub<cache::AccountStateCache>();

				auto updatedAddresses = cache.updatedAddresses();

				for (const auto& address : updatedAddresses) {
					auto iter = cache.find(address);
					auto pAccountState = iter.tryGet();

					if (!pAccountState)
						continue;

					pAccountState->Balances.maybeCleanUpSnapshots(context.Height, config, IGMRB);
				}
				return;

			};
		}
	}
	///TODO: Make NetworkConfig a core plugin
	DECLARE_OBSERVER(SnapshotCleanUp, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& holder) {
		return MAKE_OBSERVER(SnapshotCleanUp, Notification, ObserveNotification(holder));
	}
}}
