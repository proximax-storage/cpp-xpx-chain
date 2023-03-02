/**
 *
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "src/cache/SuperContractCache.h"
#include "src/cache/SuperContractCacheStorage.h"
#include "src/cache/DriveContractCache.h"
#include "src/cache/DriveContractCacheStorage.h"
#include "src/model/SuperContractEntityType.h"
#include "src/model/DeployContractTransaction.h"
#include "src/state/SuperContractEntry.h"
#include "plugins/txes/storage/src/state/DriveStateBrowserImpl.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/nodeps/Random.h"
#include "tests/TestHarness.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"

namespace catapult { namespace test {

	/// Creates test sc entry.
	state::SuperContractEntry CreateSuperContractEntry(
		Key superContractKey = test::GenerateRandomByteArray<Key>(),
		Key driveKey = test::GenerateRandomByteArray<Key>(),
		Key superContractOwnerKey = test::GenerateRandomByteArray<Key>(),
		Key executionPaymentKey = test::GenerateRandomByteArray<Key>(),
		Hash256 deploymentBaseModificationId  = test::GenerateRandomByteArray<Hash256>());

	state::DriveContractEntry CreateDriveContractEntry(
			Key drive = test::GenerateRandomByteArray<Key>(),
			Key contract = test::GenerateRandomByteArray<Key>());

	void AssertEqualAutomaticExecutionsInfo(const state::AutomaticExecutionsInfo& entry1, const state::AutomaticExecutionsInfo& entry2);
	void AssertEqualServicePayments(const std::vector<state::ServicePayment>& entry1, const std::vector<state::ServicePayment>& entry2);
	void AssertEqualRequestedCalls(const std::deque<state::ContractCall>& entry1, const std::deque<state::ContractCall>& entry2);
	void AssertEqualSuperContractData(const state::SuperContractEntry& entry1, const state::SuperContractEntry& entry2);
	void AssertEqualDriveContract(const state::DriveContractEntry& entry1, const state::DriveContractEntry& entry2);

	/// Cache factory for creating a catapult cache composed of operation cache and core caches.
	struct SuperContractCacheFactory {
	private:
		static auto CreateSubCachesWithSuperContractCache(std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder) {
			std::vector<size_t> cacheIds = {
				cache::SuperContractCache::Id,
				cache::DriveContractCache::Id,
			};

			auto maxId = *std::max_element(cacheIds.begin(), cacheIds.end());
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(maxId + 1);

			subCaches[cache::SuperContractCache::Id] = MakeSubCachePlugin<cache::SuperContractCache, cache::SuperContractCacheStorage>(pConfigHolder);
			subCaches[cache::DriveContractCache::Id] = MakeSubCachePlugin<cache::DriveContractCache, cache::DriveContractCacheStorage>(pConfigHolder);

			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create() {
			auto pConfigHolder = config::CreateMockConfigurationHolder();
			auto subCaches = CreateSubCachesWithSuperContractCache(pConfigHolder);
			CoreSystemCacheFactory::CreateSubCaches(pConfigHolder->Config(), subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			auto subCaches = CreateSubCachesWithSuperContractCache(pConfigHolder);
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};

	class DriveStateBrowserImpl : public state::DriveStateBrowser {
	public:
		uint16_t getOrderedReplicatorsCount(
				const cache::ReadOnlyCatapultCache& cache,
				const Key& driveKey) const override;

		std::set<Key> getReplicators(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const override;

		std::set<Key> getDrives(const cache::ReadOnlyCatapultCache& cache, const Key& replicatorKey) const override;

		Hash256 getDriveState(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const override;

		Hash256 getLastModificationId(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const override;
	};
}}
