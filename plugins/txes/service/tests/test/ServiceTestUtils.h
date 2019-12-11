/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/cache/DriveCache.h"
#include "src/cache/DriveCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/nodeps/Random.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace test {

	/// Creates test drive entry.
	state::DriveEntry CreateDriveEntry(
		Key key = test::GenerateRandomByteArray<Key>(),
		uint16_t billingHistoryCount = 2,
		uint16_t drivePaymentCount = 2,
		uint16_t fileCount = 2,
		uint16_t fileActionCount = 2,
		uint16_t filePaymentCount = 2,
		uint16_t replicatorCount = 2,
		uint16_t fileWithoutDepositCount = 2);

	/// Verifies that \a entry1 is equivalent to \a entry2.
	void AssertEqualDriveData(const state::DriveEntry& entry1, const state::DriveEntry& entry2);

	/// Cache factory for creating a catapult cache composed of drive cache and core caches.
	struct DriveCacheFactory {
	private:
		static auto CreateSubCachesWithDriveCache(const config::BlockchainConfiguration& config) {
			auto cacheId = cache::DriveCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			subCaches[cacheId] = MakeSubCachePlugin<cache::DriveCache, cache::DriveCacheStorage>(pConfigHolder);
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create() {
			return Create(test::MutableBlockchainConfiguration().ToConst());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateSubCachesWithDriveCache(config);
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};
}}


