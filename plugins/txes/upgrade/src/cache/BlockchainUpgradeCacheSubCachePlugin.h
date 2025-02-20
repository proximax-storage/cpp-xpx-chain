/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BlockchainUpgradeCache.h"
#include "BlockchainUpgradeCacheStorage.h"
#include "catapult/cache/SummaryAwareSubCachePluginAdapter.h"

namespace catapult { namespace cache {

	/// CacheStorage implementation for saving and loading summary network config cache data.
	class BlockchainUpgradeCacheSummaryCacheStorage : public SummaryCacheStorage<BlockchainUpgradeCache> {
	public:
		using SummaryCacheStorage<BlockchainUpgradeCache>::SummaryCacheStorage;

	public:
		void saveAll(const CatapultCacheView& cacheView, io::OutputStream& output) const override;

		void saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const override;

		void loadAll(io::InputStream& input, size_t) override;
	};

	using BaseBlockchainUpgradeCacheSubCachePlugin =
		SummaryAwareSubCachePluginAdapter<BlockchainUpgradeCache, BlockchainUpgradeCacheStorage, BlockchainUpgradeCacheSummaryCacheStorage>;

	/// Specialized network config cache sub cache plugin.
	class BlockchainUpgradeCacheSubCachePlugin : public BaseBlockchainUpgradeCacheSubCachePlugin {
	public:
		/// Creates a plugin around \a config and \a options.
		explicit BlockchainUpgradeCacheSubCachePlugin(const CacheConfiguration& config,
		                                          const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
	};
}}
