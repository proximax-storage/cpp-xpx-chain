/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "src/cache/HashCacheStorage.h"
#include "catapult/model/NetworkConfiguration.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of only the hash cache.
	struct HashCacheFactory {
		/// Creates an empty catapult cache.
		static cache::CatapultCache Create() {
			auto cacheId = cache::HashCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder();
			subCaches[cacheId] = MakeSubCachePlugin<cache::HashCache, cache::HashCacheStorage>(pConfigHolder);
			return cache::CatapultCache(std::move(subCaches));
		}
	};
}}
