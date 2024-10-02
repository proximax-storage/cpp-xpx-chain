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
		auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
		auto& bootKeyReplicatorCache = context.Cache.sub<cache::BootKeyReplicatorCache>();

		if (!bootKeyReplicatorCache.contains(processId))
			return;

		auto bootKeyReplicatorIter = bootKeyReplicatorCache.find(processId);
		const auto& replicatorKey = bootKeyReplicatorIter.get().replicatorKey();

		if (!replicatorCache.contains(replicatorKey))
			return;

		auto replicatorIter = replicatorCache.find(replicatorKey);
		const auto& replicatorEntry = replicatorIter.get();
		std::vector<std::shared_ptr<state::Drive>> updatedDrives;
		for (const auto& [ driveKey, _ ] : replicatorEntry.drives()) {
			auto eventHash = getReplicatorRemovalEventHash(context.Timestamp, context.Config.Immutable.GenerationHash, driveKey, replicatorKey);
			std::seed_seq seed(eventHash.begin(), eventHash.end());
			std::mt19937 rng(seed);

			utils::RefundDepositsOnOffboarding(driveKey, { replicatorKey }, context, m_pLiquidityProvider);
			utils::OffboardReplicatorsFromDrive(driveKey, { replicatorKey }, context, rng);
			utils::PopulateDriveWithReplicators(driveKey, context, rng);

			auto driveIter = driveCache.find(driveKey);
			auto& driveEntry = driveIter.get();

			const auto& replicators = driveEntry.replicators();
			if (replicators.find(m_pStorageState->replicatorKey()) != replicators.end()) {
				auto pDrive = utils::GetDrive(driveKey, m_pStorageState->replicatorKey(), context.Timestamp, driveCache, replicatorCache, downloadChannelCache);
				updatedDrives.push_back(std::move(pDrive));
			}
		}

		replicatorCache.remove(replicatorKey);
		bootKeyReplicatorCache.remove(processId);

		context.Notifications.push_back(std::make_unique<model::DrivesUpdateServiceNotification<1>>(std::move(updatedDrives), std::vector<Key>{}, context.Timestamp));
	}
}}
