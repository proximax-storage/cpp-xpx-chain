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
						utils::getVerificationEventHash(context.Timestamp, context.Config.Immutable.GenerationHash);

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
					std::generate_n(randomKey.begin(), sizeof(Key), rng);

					Key driveKey = treeAdapter.lowerBound(randomKey);

					CATAPULT_LOG( error ) << "will verify " << randomKey << " " << driveKey << " " << notification.Timestamp;

					if (driveKey == Key()) {
						continue;
					}

					CATAPULT_LOG( error ) << "not null key " << driveKey;

					auto& driveEntry = driveCache.find(driveKey).get();

					auto& xVerification = driveEntry.verification();

					if (xVerification && !xVerification->expired(notification.Timestamp)) {
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

					verification.Expiration = Timestamp((uint64_t) 1e15);

//					std::vector<Key> replicators;
//					replicators.reserve(driveEntry.replicators().size());
//					const auto& confirmedStates = driveEntry.confirmedStates();
//					const auto& rootHash = driveEntry.rootHash();
//					for (const auto& key : driveEntry.replicators()) {
//						auto iter = confirmedStates.find(key);
//						if (iter != confirmedStates.end() && iter->second == rootHash)
//							replicators.emplace_back(key);
//					}
//
//					uint16_t replicatorCount = replicators.size();
//					auto shardSize = 10; //pluginConfig.VerificationShardSize;
//
//					CATAPULT_LOG( error ) << "before shards";
//
//
//					std::vector<Key> r = { *driveEntry.replicators().begin() };

					std::ostringstream s;

					s << driveEntry.replicators().size() << " ";
					for (const auto& r: driveEntry.replicators()) {
						s << r << " ";
					}

//					std::set<Key> keys = { Key{{0x0A, 0x0E, 0xAC, 0x0E, 0x56, 0xFE, 0x4C, 0x05, 0x2B, 0x66, 0xD0, 0x70, 0x43, 0x46, 0x21, 0xE7, 0x47, 0x93, 0xFB, 0xF1, 0xD6, 0xF4, 0x52, 0x86, 0x89, 0x72, 0x40, 0x68, 0x1A, 0x66, 0x8B, 0xB1}} };

//					std::set<Key> keys = { Key{{1}} };

					CATAPULT_LOG( error ) << s.str();
					verification.Shards = state::Shards { driveEntry.replicators() };

					driveEntry.verification() = verification;

//					if (replicatorCount < 2 * shardSize) {
//						verification->Shards = state::Shards { replicators };
//						continue;
//					}
//
//					CATAPULT_LOG( error ) << "after shards";
//
//					std::shuffle(replicators.begin(), replicators.end(), rng);
//
//					auto shardCount = replicatorCount / shardSize;
//
//					state::Shards shards;
//					shards.resize(shardCount);
//
//					auto replicatorIt = replicators.begin();
//					for (int i = 0; i < replicatorCount / shardCount; i++) {
//						// We have the possibility to add at least one Replicator to each shard;
//						for (auto& shard : shards) {
//							shard.push_back(*replicatorIt);
//							replicatorIt++;
//						}
//					}
//
//					auto shardIt = shards.begin();
//					while (replicatorIt != replicators.end()) {
//						// The number of replicators left is less than the number of shards
//						shardIt->push_back(*replicatorIt);
//						replicatorIt++;
//						shardIt++;
//					}
//
//					verification->Shards = std::move(shards);
				}
			})) };
}} // namespace catapult::observers