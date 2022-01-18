/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/state/StorageStateImpl.h"
#include <random>
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<2>;
	using BigUint = boost::multiprecision::uint256_t;

	DECLARE_OBSERVER(StartDriveVerification, Notification)(state::StorageStateImpl& state, const cache::DriveKeyCollector& driveKeyCollector) {
		return MAKE_OBSERVER(StartDriveVerification, Notification, ([&state, &driveKeyCollector](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StartDriveVerification)");

			if (context.Height < Height(2))
				return;

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			auto verificationInterval = pluginConfig.VerificationInterval.seconds();

			auto lastBlockTimestamp = state.lastBlockElementSupplier()()->Block.Timestamp;
			auto blockGenerationTimeSeconds = (notification.Timestamp - lastBlockTimestamp).unwrap() / 1000;
			auto verificationFactor = verificationInterval / blockGenerationTimeSeconds;
			auto blockHash = Key(notification.Hash.array());

			auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();

			for (const auto& driveKey : driveKeyCollector.keys()) {
				auto driveIter = driveCache.find(driveKey);
				auto& driveEntry = driveIter.get();
				auto& verifications = driveEntry.verifications();
				if (!verifications.empty() && verifications[0].Expired)
					verifications.clear();

				if (!verifications.empty()) {
					auto& verification = verifications[0];
					if (verification.Expired) {
						verifications.clear();
					} else {
						if (verification.Expiration > lastBlockTimestamp)
							verification.Expired = true;
						continue;
					}
				}

				auto driveKeyXorBlockHash = driveKey ^ blockHash;
				std::ostringstream out;
				out << "0x" << driveKeyXorBlockHash;
				BigUint value(out.str());
				if (value % verificationFactor)
					continue;

				std::vector<Key> replicators;
				replicators.reserve(driveEntry.replicators().size());
				const auto& confirmedStates = driveEntry.confirmedStates();
				const auto& rootHash = driveEntry.rootHash();
				for (const auto& key : driveEntry.replicators()) {
					auto iter = confirmedStates.find(key);
					if (iter != confirmedStates.end() && iter->second == rootHash)
						replicators.emplace_back(key);
				}

				auto timeoutMinutes = pluginConfig.VerificationExpirationCoefficient * driveEntry.usedSize() + pluginConfig.VerificationExpirationConstant;
				auto expiration = lastBlockTimestamp + Timestamp(timeoutMinutes * 60 * 1000);

				uint16_t replicatorCount = driveEntry.replicators().size();
				if (replicatorCount < 2 * pluginConfig.ShardSize) {
					verifications.emplace_back(state::Verification{ notification.Hash, expiration, false, state::Shards{ replicators }});
					continue;
				}

				std::seed_seq hashSeed(driveKeyXorBlockHash.begin(), driveKeyXorBlockHash.end());
				std::shuffle(replicators.begin(), replicators.end(), std::mt19937_64(hashSeed));

				replicatorCount = replicators.size();
				auto shardSize = pluginConfig.ShardSize;
				auto shardCount = replicatorCount / shardSize;
				auto lastShardSize = replicatorCount % shardSize;
				std::vector<std::pair<uint16_t, uint8_t>> distribution;
				if (lastShardSize == 0) {
					distribution.push_back(std::make_pair(shardCount, shardSize));
				} else {
					auto additionalReplicators = lastShardSize / shardCount;
					auto remainder = lastShardSize % shardCount;
					if (remainder == 0) {
						distribution.push_back(std::make_pair(shardCount, shardSize + additionalReplicators));
					} else {
						distribution.push_back(std::make_pair(remainder, shardSize + additionalReplicators + 1));
						distribution.push_back(std::make_pair(shardCount - remainder, shardSize + additionalReplicators));
					}
				}

				state::Shards shards;
				shards.reserve(shardCount);
				auto index = 0u;
				for (const auto& pair : distribution) {
					for (auto i = 0u; i < pair.first; ++i) {
						shards.push_back({});
						auto& shard = shards.back();
						shard.reserve(pair.second);
						for (auto k = 0u; k < pair.second; ++k) {
							shard.emplace_back(replicators[index++]);
						}
					}
				}

				verifications.emplace_back(state::Verification{ notification.Hash, expiration, false, std::move(shards) });
			}
        }))
	};
}}