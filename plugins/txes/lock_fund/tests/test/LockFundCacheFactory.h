/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/config/BlockchainConfiguration.h"
#include "catapult/cache/CacheConfiguration.h"
#include "catapult/cache/CatapultCache.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of all core sub caches plus LockFundCache.
	struct LockFundCacheFactory{

		/// Creates an empty catapult cache.
		static cache::CatapultCache Create();

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config);

		/// Adds all core sub caches initialized with \a config to \a subCaches.
		static void CreateSubCaches(
				const config::BlockchainConfiguration& config,
				std::vector<std::unique_ptr<cache::SubCachePlugin>>& subCaches);

		/// Adds all core sub caches initialized with \a config and \a cacheConfig to \a subCaches.
		static void CreateSubCaches(
				const config::BlockchainConfiguration& config,
				const cache::CacheConfiguration& cacheConfig,
				std::vector<std::unique_ptr<cache::SubCachePlugin>>& subCaches);
	};

}}
