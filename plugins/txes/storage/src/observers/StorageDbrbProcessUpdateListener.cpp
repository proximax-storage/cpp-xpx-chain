/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/BootKeyReplicatorCache.h"
#include "catapult/utils/StorageUtils.h"

namespace catapult { namespace observers {

	void StorageDbrbProcessUpdateListener::OnDbrbProcessRemoved(ObserverContext& context, const dbrb::ProcessId& processId) const {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StorageDbrbProcessUpdateListener)");

		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		if (!pluginConfig.EnableReplicatorBootKeyBinding)
			return;

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		auto& bootKeyReplicatorCache = context.Cache.sub<cache::BootKeyReplicatorCache>();

		if (!bootKeyReplicatorCache.contains(processId))
			return;

		auto bootKeyReplicatorIter = bootKeyReplicatorCache.find(processId);
		const auto& replicatorKey = bootKeyReplicatorIter.get().replicatorKey();

		if (!replicatorCache.contains(replicatorKey))
			return;

		auto replicatorIter = replicatorCache.find(replicatorKey);
		const auto& replicatorEntry = replicatorIter.get();
		for (const auto& [ driveKey, _ ] : replicatorEntry.drives()) {
			auto eventHash = getReplicatorRemovalEventHash(context.Timestamp, context.Config.Immutable.GenerationHash, driveKey, replicatorKey);
			std::seed_seq seed(eventHash.begin(), eventHash.end());
			std::mt19937 rng(seed);

			utils::RefundDepositsOnOffboarding(driveKey, { replicatorKey }, context, m_pLiquidityProvider);
			utils::OffboardReplicatorsFromDrive(driveKey, { replicatorKey }, context, rng);
			utils::PopulateDriveWithReplicators(driveKey, context, rng);
		}

		replicatorCache.remove(replicatorKey);
		bootKeyReplicatorCache.remove(processId);
	}
}}
