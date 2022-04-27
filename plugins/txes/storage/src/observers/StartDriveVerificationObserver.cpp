/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/AVLTree.h"
#include "src/catapult/utils/StorageUtils.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	void Observe(const Notification& notification, ObserverContext& context, state::StorageState& state) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StartDriveVerification)");

		if (context.Height < Height(2))
			return;

		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		auto verificationInterval = pluginConfig.VerificationInterval.seconds();

		auto pLastBlockElement = state.lastBlockElementSupplier()();
		auto lastBlockTimestamp = pLastBlockElement->Block.Timestamp;
		auto blockGenerationTimeSeconds = (notification.Timestamp - lastBlockTimestamp).unwrap() / 1000;

		auto verificationFactor = verificationInterval / std::max(blockGenerationTimeSeconds, 1ul);

		auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();
		auto totalDrives = driveCache.size();
		auto drivesToVerify = totalDrives / std::max(verificationFactor, 1ul) + 1; // + 1 instead of ceil

		utils::AVLTreeAdapter treeAdapter(
			context.Cache.template sub<cache::QueueCache>(),
			state::DriveVerificationsTree,
			[&driveCache](const Key& key) -> state::AVLTreeNode {
				return driveCache.find(key).get().verificationNode();
			},
			[&driveCache](const Key& key, const state::AVLTreeNode& node) {
				driveCache.find(key).get().verificationNode() = node;
			});

		// Creating unique eventHash for the observer
		Hash256 eventHash = utils::getVerificationEventHash(notification.Timestamp, context.Config.Immutable.GenerationHash);

		std::seed_seq seed(eventHash.begin(), eventHash.end());
		std::mt19937 rng(seed);

		for (uint i = 0; i < drivesToVerify; i++) {
			Key randomKey = Key(eventHash.array());
			std::generate_n(randomKey.begin(), sizeof(Key), rng);

			Key driveKey = treeAdapter.lowerBound(randomKey);
			if (driveKey == Key()) {
				continue;
			}

			auto iter = driveCache.find(driveKey);
			auto& driveEntry = iter.get();
			if (driveEntry.verification() && !driveEntry.verification()->expired(notification.Timestamp)) {
				// The Verification Has Not Expired Yet
				continue;
			}

			state::Verification verification;
			verification.VerificationTrigger = eventHash;

			auto timeoutMinutes =
				pluginConfig.VerificationExpirationCoefficient *
					utils::FileSize::FromBytes(driveEntry.usedSizeBytes()).gigabytesCeil() +
				pluginConfig.VerificationExpirationConstant;
			verification.Expiration = notification.Timestamp + Timestamp(timeoutMinutes * 60 * 1000);

			std::vector<Key> replicators;
			replicators.reserve(driveEntry.replicators().size());
			const auto& confirmedStates = driveEntry.confirmedStates();
			const auto& rootHash = driveEntry.rootHash();
			for (const auto& key : driveEntry.replicators()) {
				auto iter = confirmedStates.find(key);
				if (iter != confirmedStates.end() && iter->second == rootHash)
					replicators.emplace_back(key);
			}

			uint16_t replicatorCount = replicators.size();
			auto shardSize = pluginConfig.ShardSize / 2;

			if (replicatorCount < 2 * shardSize) {
				verification.Shards = state::Shards{
					std::set<Key>(std::make_move_iterator(replicators.begin()),
					std::make_move_iterator(replicators.end()))};
				continue;
			}

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

			verification.Shards = std::move(shards);
			driveEntry.verification() = verification;
		}
	}

	DECLARE_OBSERVER(StartDriveVerification, Notification)(state::StorageState& state) {
		return MAKE_OBSERVER(StartDriveVerification, Notification, ([&state](const Notification& notification, ObserverContext& context) {
			Observe(notification, context, state);
		}))
	}
}}