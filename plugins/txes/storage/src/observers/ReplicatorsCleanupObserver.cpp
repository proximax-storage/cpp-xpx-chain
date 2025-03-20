/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/utils/StorageUtils.h"

namespace catapult { namespace observers {

	DECLARE_OBSERVER(ReplicatorsCleanupV1, model::ReplicatorsCleanupNotification<1>)(const std::unique_ptr<LiquidityProviderExchangeObserver>& pLiquidityProvider) {
		return MAKE_OBSERVER(ReplicatorsCleanupV1, model::ReplicatorsCleanupNotification<1>, ([&pLiquidityProvider](const model::ReplicatorsCleanupNotification<1>& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorsCleanupV1)");

			auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
			auto pReplicatorKey = notification.ReplicatorKeysPtr;
			for (auto i = 0u; i < notification.ReplicatorCount; ++i, ++pReplicatorKey) {
				auto replicatorIter = replicatorCache.find(*pReplicatorKey);
				const auto& replicatorEntry = replicatorIter.get();
				for (const auto& [ driveKey, _ ] : replicatorEntry.drives()) {
					auto eventHash = getReplicatorRemovalEventHash(context.Timestamp, context.Config.Immutable.GenerationHash, driveKey, *pReplicatorKey);
					std::seed_seq seed(eventHash.begin(), eventHash.end());
					std::mt19937 rng(seed);

					utils::RefundDepositsOnOffboarding(driveKey, { *pReplicatorKey }, context, pLiquidityProvider);
					utils::OffboardReplicatorsFromDrive(driveKey, { *pReplicatorKey }, context, rng);
					utils::PopulateDriveWithReplicators(driveKey, context, rng);
				}

				replicatorCache.remove(*pReplicatorKey);
			}
        }))
	};

	DECLARE_OBSERVER(ReplicatorsCleanupV2, model::ReplicatorsCleanupNotification<2>)() {
		return MAKE_OBSERVER(ReplicatorsCleanupV2, model::ReplicatorsCleanupNotification<2>, ([](const model::ReplicatorsCleanupNotification<2>& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorsCleanupV2)");

			auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
			auto pReplicatorKey = notification.ReplicatorKeysPtr;
			for (auto i = 0u; i < notification.ReplicatorCount; ++i, ++pReplicatorKey) {
				auto replicatorIter = replicatorCache.find(*pReplicatorKey);
				auto& replicatorEntry = replicatorIter.get();
				replicatorEntry.drives().clear();
			}
        }))
	};
}}
