/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::ReplicatorOffboardingNotification<1>;
	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

	DECLARE_OBSERVER(ReplicatorOffboarding, Notification)(const std::shared_ptr<DriveQueue>& pDriveQueue) {
		return MAKE_OBSERVER(ReplicatorOffboarding, Notification, ([pDriveQueue](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorOffboarding)");

  			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();
		  	auto driveIter = driveCache.find(notification.DriveKey);
		  	auto& driveEntry = driveIter.get();

		  	driveEntry.offboardingReplicators().emplace(notification.PublicKey);

			if (driveEntry.replicators().size() < driveEntry.replicatorCount()) {
				DriveQueue originalQueue = *pDriveQueue;
				DriveQueue newQueue;
				newQueue.emplace(
						notification.DriveKey,
						utils::CalculateDrivePriority(driveEntry, pluginConfig.MinReplicatorCount));
				while (!originalQueue.empty()) {
					const auto drivePriorityPair = originalQueue.top();
					const auto& driveKey = drivePriorityPair.first;
					originalQueue.pop();

					if (driveKey != notification.DriveKey)
						newQueue.push(drivePriorityPair);
				}
				*pDriveQueue = std::move(newQueue);
			}

			auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
			auto replicatorIter = replicatorCache.find(notification.PublicKey);
			auto& replicatorEntry = replicatorIter.get();

			auto& cache = context.Cache.template sub<cache::AccountStateCache>();
			auto accountIter = cache.find(notification.PublicKey);
			auto& replicatorState = accountIter.get();

			// Storage deposit equals to the remaining capacity plus the sum of drive
			// sizes that the replicator serves.
			auto storageDepositReturn = replicatorEntry.capacity().unwrap();

			// Streaming Deposit Slashing equals 2 * min(u1, u2) where
			// u1 - the UsedDriveSize according to the last approved by the Replicator modification
			// u2 - the UsedDriveSize according to the last approved modification on the Drive.
			uint64_t streamingDepositSlashing = 0;

			const auto currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
			for(const auto& iter : replicatorEntry.drives()){
				auto driveIter = driveCache.find(iter.first);
				const auto& drive = driveIter.get();
				storageDepositReturn += drive.size();

				auto& confirmedUsedSizes = drive.confirmedUsedSizes();
				auto sizeIter = confirmedUsedSizes.find(notification.PublicKey);
				streamingDepositSlashing += (confirmedUsedSizes.end() != sizeIter) ?
					2 * std::min(sizeIter->second, drive.usedSize()) :
					2 * drive.usedSize();
			}

			// Streaming deposit return = streaming deposit - streaming deposit slashing
			auto streamingDeposit = 2 * storageDepositReturn;
			if (streamingDeposit < streamingDepositSlashing)
				CATAPULT_THROW_RUNTIME_ERROR_2("streaming deposit slashing exceeds streaming deposit", streamingDeposit, streamingDepositSlashing);
			auto streamingDepositReturn = streamingDeposit - streamingDepositSlashing;

			// Swap storage unit to xpx
			replicatorState.Balances.credit(currencyMosaicId, Amount(storageDepositReturn), context.Height);
			// Swap streaming unit to xpx
			replicatorState.Balances.credit(currencyMosaicId, Amount(streamingDepositReturn), context.Height);
		}))
	}
}}