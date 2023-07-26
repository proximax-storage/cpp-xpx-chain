/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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
#include "src/cache/MosaicRestrictionCache.h"
#include "src/cache/MosaicRestrictionCacheStorage.h"
#include "tests/test/MosaicRestrictionTestUtils.h"
#include "catapult/config/BlockchainConfiguration.h"
#include "tests/test/cache/CacheTestUtils.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache containing at least the mosaic restriction cache.
	struct MosaicRestrictionCacheFactory {
	public:
		/// Creates an empty catapult cache with optional \a networkIdentifier.
		static cache::CatapultCache Create(model::NetworkIdentifier networkIdentifier = model::NetworkIdentifier::Zero, uint maxMosaicRestrictionValues = 10) {
			auto cacheId = cache::MosaicRestrictionCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			subCaches[cacheId] = MakeSubCachePlugin<cache::MosaicRestrictionCache, cache::MosaicRestrictionCacheStorage>(test::CreateMosaicRestrictionConfigHolder(maxMosaicRestrictionValues, networkIdentifier)
					);
			return cache::CatapultCache(std::move(subCaches));
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config, uint maxMosaicRestrictionValues = 10) {
			return Create(config.Immutable.NetworkIdentifier, maxMosaicRestrictionValues);
		}
	};
}}
