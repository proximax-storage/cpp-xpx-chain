/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include "Observers.h"
#include "src/utils/StorageUtils.h"

namespace catapult { namespace observers {

	using Notification = model::EndDriveVerificationNotification<1>;

	DECLARE_OBSERVER(EndDriveVerification, Notification)() {
		return MAKE_OBSERVER(EndDriveVerification, Notification, ([](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (EndDriveVerification)");

			// Find median opinion for every Prover
			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();

		  	auto& shards = driveEntry.verification()->Shards;
			auto& shardSet = shards[notification.ShardId];
			const std::vector<Key> shardVec(shardSet.begin(), shardSet.end());
		  	const auto opinionByteCount = (notification.JudgingKeyCount * notification.KeyCount + 7) / 8;
		  	const boost::dynamic_bitset<uint8_t> opinions(notification.OpinionsPtr, notification.OpinionsPtr + opinionByteCount);
			std::set<Key> offboardingReplicators;
		  	size_t voluntarilyOffboardingCount = 0;
		  	auto storageDepositSlashing = 0;

			for (auto i = 0; i < notification.KeyCount; ++i) {
				uint8_t result = 0;
				for (auto j = 0; j < notification.JudgingKeyCount; ++j)
					result += opinions[j * notification.KeyCount + i];

				const auto& replicatorKey = shardVec[i];
				if (result >= notification.JudgingKeyCount / 2) {
					// If the replicator passes validation and is queued for offboarding,
					// increment voluntarilyOffboardingCount
					const auto& keys = driveEntry.offboardingReplicators();
					if (std::find(keys.begin(), keys.end(), replicatorKey) != keys.end()) {
						++voluntarilyOffboardingCount;
					}
				} else {
					// Count deposited Storage mosaics and include replicator into offboardingReplicators
					storageDepositSlashing += driveEntry.size();
					offboardingReplicators.insert(replicatorKey);
				}
			}

		  	const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			const auto requiredReplicatorsCount = pluginConfig.MinReplicatorCount * 2 / 3 + 1;
			const auto maxVoluntarilyOffboardingCount =
					driveEntry.replicators().size() < offboardingReplicators.size() + requiredReplicatorsCount ?
					0ul :
					driveEntry.replicators().size() - offboardingReplicators.size() - requiredReplicatorsCount;
			voluntarilyOffboardingCount = std::min(voluntarilyOffboardingCount, maxVoluntarilyOffboardingCount);
			const auto voluntarilyOffboardingReplicators = std::set<Key>(
					driveEntry.offboardingReplicators().begin(),
					driveEntry.offboardingReplicators().begin() + voluntarilyOffboardingCount);
			offboardingReplicators.insert(voluntarilyOffboardingReplicators.begin(),
										  voluntarilyOffboardingReplicators.end());

			std::seed_seq seed(notification.Seed.begin(), notification.Seed.end());
			std::mt19937 rng(seed);

		  	utils::RefundDepositsOnOffboarding(notification.DriveKey, voluntarilyOffboardingReplicators, context);
			utils::OffboardReplicatorsFromDrive(notification.DriveKey, offboardingReplicators, context, rng);

			shardSet.clear();
		  	bool verificationCompleted = true;
		  	for (const auto& shard : shards) {
			  	if (!shard.empty()) {
					verificationCompleted = false;
				  	break;
			  	}
		  	}

			if (verificationCompleted)
				driveEntry.verification().reset();

			if (storageDepositSlashing == 0)
				return;

			// Split storage deposit slashing between remaining replicators
			auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
			auto& voidState = utils::getVoidState(context);

			auto storageDepositSlashingShare = Amount(storageDepositSlashing / driveEntry.replicators().size());
			const auto currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
			const auto storageMosaicId = context.Config.Immutable.StorageMosaicId;

			for (const auto& replicatorKey : driveEntry.replicators()) {
				auto accountIter = accountStateCache.find(replicatorKey);
				auto& replicatorAccountState = accountIter.get();
				voidState.Balances.debit(storageMosaicId, storageDepositSlashingShare, context.Height);
				replicatorAccountState.Balances.credit(currencyMosaicId, storageDepositSlashingShare, context.Height);
			}

			// Streaming deposits of failed provers remain on drive's account

		  	// Populate the drive AFTER storage deposit slashing is made
			utils::PopulateDriveWithReplicators(notification.DriveKey, context, rng);
    	}))
	}
}}