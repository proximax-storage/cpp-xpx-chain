/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/StorageUtils.h"

namespace catapult { namespace observers {

	using Notification = model::EndDriveVerificationNotification<1>;
	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

	DECLARE_OBSERVER(EndDriveVerification, Notification)(const std::shared_ptr<cache::ReplicatorKeyCollector>& pKeyCollector, const std::shared_ptr<DriveQueue>& pDriveQueue) {
		return MAKE_OBSERVER(EndDriveVerification, Notification, ([pKeyCollector, pDriveQueue](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (EndDriveVerification)");

			// Find median opinion for every Prover
			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();
			auto storageDepositSlashing = 0;
			std::set<Key> offboardingReplicators;
		  	std::set<Key> offboardingReplicatorsWithRefund;
			for (auto i = 0; i < notification.KeyCount; ++i) {
				uint8_t result = 0;
				for (auto j = 0; j < notification.JudgingKeyCount; ++j)
					result += notification.OpinionsPtr[i + j * (notification.KeyCount - 1)];

				if (result >= notification.JudgingKeyCount / 2) {
					// If the replicator passes validation and is queued for offboarding,
					// include him into offboardingReplicators and offboardingReplicatorsWithRefund
					if (driveEntry.offboardingReplicators().count(notification.PublicKeysPtr[i])) {
						offboardingReplicators.insert(notification.PublicKeysPtr[i]);
						offboardingReplicatorsWithRefund.insert(notification.PublicKeysPtr[i]);
					}
				} else {
					// Count deposited Storage mosaics and include replicator into offboardingReplicators
					storageDepositSlashing += driveEntry.size();
					offboardingReplicators.insert(notification.PublicKeysPtr[i]);
				}
			}

			std::seed_seq seed(notification.Seed.begin(), notification.Seed.end());
			std::mt19937 rng(seed);

		  	utils::RefundDepositsToReplicators(notification.DriveKey, offboardingReplicatorsWithRefund, context);
			utils::OffboardReplicatorsFromDrive(notification.DriveKey, offboardingReplicators, context, rng);
			utils::PopulateDriveWithReplicators(notification.DriveKey, pKeyCollector, pDriveQueue, context, rng);
			utils::AssignReplicatorsToQueuedDrives(offboardingReplicators, pDriveQueue, context, rng);

			const auto& replicatorKey = notification.PublicKeysPtr[0];
			auto& shards = driveEntry.verification()->Shards;
			for (auto iter = shards.begin(); iter != shards.end(); ++iter) {
				const auto& shard = *iter;

				bool found = false;
				for (const auto& key : shard) {
					if (key == replicatorKey) {
						found = true;
						break;
					}
				}

				if (found) {
					shards.erase(iter);
					break;
				}
			}

			if (shards.empty())
				driveEntry.verification().reset();

			if (storageDepositSlashing == 0)
				return;

			// Split storage deposit slashing between remaining replicators
			auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
			auto accountIter = accountStateCache.find(notification.DriveKey);
			auto& driveAccountState = accountIter.get();
			auto storageDepositSlashingShare = Amount(storageDepositSlashing / driveEntry.replicators().size());
			const auto storageMosaicId = context.Config.Immutable.StorageMosaicId;

			for (const auto& replicatorKey : driveEntry.replicators()) {
				accountIter = accountStateCache.find(replicatorKey);
				auto& replicatorAccountState = accountIter.get();
				driveAccountState.Balances.debit(storageMosaicId, storageDepositSlashingShare, context.Height);
				replicatorAccountState.Balances.credit(storageMosaicId, storageDepositSlashingShare, context.Height);
			}

			// Streaming deposits of failed provers remain on drive's account
    	}))
	}
}}