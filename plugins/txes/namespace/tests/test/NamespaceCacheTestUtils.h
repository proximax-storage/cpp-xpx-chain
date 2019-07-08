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
#include "src/cache/NamespaceCache.h"
#include "src/cache/NamespaceCacheStorage.h"
#include "src/config/NamespaceConfiguration.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of only the namespace cache.
	struct NamespaceCacheFactory {
		/// Creates an empty catapult cache around \a gracePeriodDuration.
		static cache::CatapultCache Create(const model::BlockChainConfiguration& blockChainConfig, BlockDuration gracePeriodDuration) {
			auto cacheId = cache::NamespaceCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);

			auto pluginConfig = config::NamespaceConfiguration::Uninitialized();
			pluginConfig.NamespaceGracePeriodDuration = utils::BlockSpan::FromHours(gracePeriodDuration.unwrap());
			const_cast<model::BlockChainConfiguration&>(blockChainConfig).BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
			const_cast<model::BlockChainConfiguration&>(blockChainConfig).SetPluginConfiguration("catapult.plugins.namespace", pluginConfig);
			auto pConfigHolder = std::make_shared<config::MockLocalNodeConfigurationHolder>();
			pConfigHolder->SetBlockChainConfig(blockChainConfig);
			auto options = cache::NamespaceCacheTypes::Options{ pConfigHolder };
			subCaches[cacheId] = MakeSubCachePlugin<cache::NamespaceCache, cache::NamespaceCacheStorage>(options);
			return cache::CatapultCache(std::move(subCaches));
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const model::BlockChainConfiguration& config) {
			auto configIter = config.Plugins.find("namespace::ex");
			return config.Plugins.cend() != configIter
					? Create(config, BlockDuration(configIter->second.get<uint64_t>({ "", "gracePeriodDuration" })))
					: Create(config, BlockDuration(10));
		}
	};

	/// Asserts that \a cache has the expected sizes (\a size, \a activeSize and \a deepSize).
	void AssertCacheSizes(const cache::NamespaceCacheView& cache, size_t size, size_t activeSize, size_t deepSize);

	/// Asserts that \a cache has the expected sizes (\a size, \a activeSize and \a deepSize).
	void AssertCacheSizes(const cache::NamespaceCacheDelta& cache, size_t size, size_t activeSize, size_t deepSize);

	/// Asserts that \a cache exactly contains the namespace ids in \a expectedIds.
	void AssertCacheContents(const cache::NamespaceCache& cache, std::initializer_list<NamespaceId::ValueType> expectedIds);

	/// Asserts that \a cache exactly contains the namespace ids in \a expectedIds.
	void AssertCacheContents(const cache::NamespaceCacheView& cache, std::initializer_list<NamespaceId::ValueType> expectedIds);

	/// Asserts that \a cache exactly contains the namespace ids in \a expectedIds.
	void AssertCacheContents(const cache::NamespaceCacheDelta& cache, std::initializer_list<NamespaceId::ValueType> expectedIds);
}}
