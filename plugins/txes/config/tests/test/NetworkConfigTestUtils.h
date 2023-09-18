/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/cache_core/BlockDifficultyCache.h"
#include "plugins/txes/config/src/cache/NetworkConfigCache.h"
#include "plugins/txes/config/src/cache/NetworkConfigCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace cache { class CatapultCacheDelta; } }

namespace catapult { namespace test {

	std::string networkConfig();
	std::string supportedVersions();

	struct NetworkConfigCacheFactory {
	private:
		static auto CreateSubCachesWithDriveCache(const config::BlockchainConfiguration& config) {
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cache::BlockDifficultyCache::Id + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolderWithNemesisConfig(config);
			subCaches[cache::NetworkConfigCache::Id] = test::MakeSubCachePlugin<cache::NetworkConfigCache, cache::NetworkConfigCacheStorage>(pConfigHolder);
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


