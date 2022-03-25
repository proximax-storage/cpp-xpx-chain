/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/catapult/state/StorageState.h"
#include "src/utils/AVLTree.h"
#include "src/utils/StorageUtils.h"
#include "src/catapult/utils/StorageUtils.h"
#include <random>
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<2>;
	using BigUint = boost::multiprecision::uint256_t;

	DECLARE_OBSERVER(StartDriveVerification, Notification)
	(state::StorageState& state) { return MAKE_OBSERVER(
			StartDriveVerification,
			Notification,
			([&state](const Notification& notification, ObserverContext& context) {
				if (NotifyMode::Rollback == context.Mode)
					CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StartDriveVerification)");

				if (context.Height < Height(2))
					return;

				const auto& pluginConfig =
						context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
				auto verificationInterval = pluginConfig.VerificationInterval.seconds();

				auto lastBlockTimestamp = state.lastBlockElementSupplier()()->Block.Timestamp;
				auto blockGenerationTimeSeconds = (notification.Timestamp - lastBlockTimestamp).unwrap() / 1000;

				auto verificationFactor = verificationInterval / std::max(blockGenerationTimeSeconds, 1ul);
				auto blockHash = Key(notification.Hash.array());

				auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();

				auto totalDrives = driveCache.size();
				auto drivesToVerify = totalDrives / std::max(verificationFactor, 1ul) + 1; // + 1 instead of ceil

				// Creating unique eventHash for the observer
				Hash256 eventHash =
						utils::getVerificationEventHash(notification.Hash, context.Config.Immutable.GenerationHash);

				std::seed_seq seed(eventHash.begin(), eventHash.end());
				std::mt19937 rng(seed);

				auto& queueCache = context.Cache.template sub<cache::QueueCache>();

				utils::AVLTreeAdapter<Key> treeAdapter(
						queueCache,
						state::DriveVerificationsTree,
						[](const Key& key) { return key; },
						[&](const Key& key) -> state::AVLTreeNode& {
							return driveCache.find(key).get().verificationNode();
						});

				for (uint i = 0; i < drivesToVerify; i++) {
					Key randomKey;
//					std::generate_n(randomKey.begin(), sizeof(Key), rng);

					Key driveKey = treeAdapter.lowerBound(randomKey);

					CATAPULT_LOG( error ) << "will verify " << randomKey << " " << driveKey << " " << notification.Timestamp;

					if (driveKey == Key()) {
						continue;
					}
					auto& driveEntry = driveCache.find(driveKey).get();

					auto& verification = driveEntry.verification();

					if (verification && !verification->expired(notification.Timestamp)) {
						// The Verification Has Not Expired Yet
						continue;
					}

					verification = state::Verification();

					verification->VerificationTrigger = eventHash;

					auto timeoutMinutes =
							pluginConfig.VerificationExpirationCoefficient *
									utils::FileSize::FromBytes(driveEntry.usedSizeBytes()).gigabytesCeil() +
							pluginConfig.VerificationExpirationConstant;
//					verification->Expiration = notification.Timestamp + Timestamp(timeoutMinutes * 60 * 1000);

					verification->Expiration = Timestamp((uint64_t) 1e15);

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
					auto shardSize = 10; //pluginConfig.VerificationShardSize;

					if (replicatorCount < 2 * shardSize) {
						verification->Shards = state::Shards { replicators };
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
							shard.push_back(*replicatorIt);
							replicatorIt++;
						}
					}

					auto shardIt = shards.begin();
					while (replicatorIt != replicators.end()) {
						// The number of replicators left is less than the number of shards
						shardIt->push_back(*replicatorIt);
						replicatorIt++;
						shardIt++;
					}

					verification->Shards = std::move(shards);
				}
			})) };
}} // namespace catapult::observers