/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/AVLTree.h"
#include "catapult/utils/StorageUtils.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	void Observe(const Notification& notification, ObserverContext& context, const std::shared_ptr<state::StorageState>& pStorageState) {
		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		if (!pluginConfig.Enabled || context.Height < Height(2))
			return;

		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StartDriveVerification)");

		uint64_t verificationInterval = pluginConfig.VerificationInterval.seconds();

		auto pLastBlockElement = pStorageState->lastBlockElementSupplier()();
		auto lastBlockTimestamp = pLastBlockElement->Block.Timestamp;
		uint64_t blockGenerationTimeSeconds = (notification.Timestamp - lastBlockTimestamp).unwrap() / 1000;

		uint64_t verificationFactor = std::max(verificationInterval / std::max(blockGenerationTimeSeconds, uint64_t(1)), uint64_t(1));

		auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();

		utils::AVLTreeAdapter<Key> treeAdapter(
				context.Cache.template sub<cache::QueueCache>(),
				state::DriveVerificationsTree,
				[](const Key& key) { return key; },
				[&driveCache](const Key& key) -> state::AVLTreeNode {
					auto iter = driveCache.find(key);
					const auto& driveEntry = iter.get();
					return driveEntry.verificationNode();
				},
				[&driveCache](const Key& key, const state::AVLTreeNode& node) {
					auto iter = driveCache.find(key);
					auto& driveEntry = iter.get();
					driveEntry.verificationNode() = node;
				});

		auto totalDrives = treeAdapter.size();

		if (totalDrives == 0) {
			return;
		}

		auto guaranteedVerifications = totalDrives / verificationFactor;

		// Creating unique eventHash for the observer
		Hash256 eventHash = utils::getVerificationEventHash(notification.Timestamp, context.Config.Immutable.GenerationHash);

		std::seed_seq seed(eventHash.begin(), eventHash.end());
		std::mt19937 rng(seed);

		// There can be maximum 1 additional verification. It matters for the case when there are few drives
		auto r = rng() % verificationFactor;
		uint32_t additionalVerifications = r < (totalDrives % verificationFactor) ? 1 : 0;

		auto drivesToVerify = guaranteedVerifications + additionalVerifications;

		std::unordered_map<Key, std::shared_ptr<state::DriveVerification>, utils::ArrayHasher<Key>> verifications;
		for (uint i = 0; i < drivesToVerify; i++) {

			uint32_t index = rng() % totalDrives;

			Key driveKey = treeAdapter.orderStatistics(index);

			auto iter = driveCache.find(driveKey);
			auto& driveEntry = iter.get();


			// The drive hasn't been modified yet
			if (driveEntry.rootHash() == Hash256())
				continue;

			// The verification has not expired yet
			if (driveEntry.verification() && !driveEntry.verification()->expired(notification.Timestamp))
				continue;

			// The drive doesn't have enough replicators to work
			if (driveEntry.replicators().size() < pluginConfig.MinReplicatorCount)
				continue;


			auto& verification = driveEntry.verification();
			verification = state::Verification();

			verification->VerificationTrigger = eventHash;

			auto timeoutMinutes =
				static_cast<uint64_t>(pluginConfig.VerificationExpirationCoefficient *
					static_cast<double>(utils::FileSize::FromBytes(driveEntry.usedSizeBytes()).gigabytesCeil()) +
				pluginConfig.VerificationExpirationConstant);
			verification->Expiration = notification.Timestamp + Timestamp(timeoutMinutes * 60 * 1000);
			verification->Duration = uint64_t(timeoutMinutes * 60 * 1000);

			std::vector<Key> replicators;
			replicators.reserve(driveEntry.replicators().size());

			for (const auto& key : driveEntry.replicators()) {
				const auto& confirmedStorageInfo = driveEntry.confirmedStorageInfos()[key];
				if (confirmedStorageInfo.ConfirmedStorageSince) {
					replicators.emplace_back(key);
				}
			}

			uint16_t replicatorCount = replicators.size();
			auto shardSize = pluginConfig.ShardSize / 2;

			if (replicatorCount < 2 * shardSize) {
				verification->Shards = state::Shards{
					std::set<Key>(std::make_move_iterator(replicators.begin()),
					std::make_move_iterator(replicators.end()))};
			} else {
				std::shuffle(replicators.begin(), replicators.end(), rng);

				auto shardCount = replicatorCount / shardSize;

				state::Shards shards;
				shards.resize(shardCount);

				auto replicatorIt = replicators.begin();
				for (int i = 0; i < replicatorCount / shardCount; i++) {
					// We have the possibility to add at least one Replicator to each shard;
					for (auto& shard : shards) {
						shard.emplace(*replicatorIt);
						replicatorIt++;
					}
				}

				auto shardIt = shards.begin();
				while (replicatorIt != replicators.end()) {
					// The number of replicators left is less than the number of shards
					shardIt->emplace(*replicatorIt);
					replicatorIt++;
					shardIt++;
				}

				verification->Shards = std::move(shards);
			}

			const auto& driveReplicators = driveEntry.replicators();
			if (driveReplicators.find(pStorageState->replicatorKey()) == driveReplicators.end())
				return;

			verifications.emplace(driveEntry.key(), utils::GetDriveVerification(driveEntry, context.Timestamp));
		}

		if (!verifications.empty())
			context.Notifications.push_back(std::make_unique<model::StartDriveVerificationServiceNotification<1>>(std::move(verifications)));
	}

	DECLARE_OBSERVER(StartDriveVerification, Notification)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(StartDriveVerification, Notification, ([pStorageState](const Notification& notification, ObserverContext& context) {
			Observe(notification, context, pStorageState);
		}))
	}
}}