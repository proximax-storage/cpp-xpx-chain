/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LiquidityProviderCache.h"
#include "LiquidityProviderCacheStorage.h"
#include "catapult/cache/SummaryAwareSubCachePluginAdapter.h"

namespace catapult { namespace cache {

	/// CacheStorage implementation for saving and loading summary network config cache data.
	class LiquidityProviderCacheSummaryCacheStorage : public SummaryCacheStorage<LiquidityProviderCache> {
	public:
		using SummaryCacheStorage<LiquidityProviderCache>::SummaryCacheStorage;

	public:
		void saveAll(const CatapultCacheView& cacheView, io::OutputStream& output) const override;

		void saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const override;

		void loadAll(io::InputStream& input, size_t) override;
	};

	using BaseLiquidityProviderCacheSubCachePlugin =
		SummaryAwareSubCachePluginAdapter<
			LiquidityProviderCache,
			LiquidityProviderCacheStorage, LiquidityProviderCacheSummaryCacheStorage>;

	/// Specialized network config cache sub cache plugin.
	class LiquidityProviderCacheSubCachePlugin : public BaseLiquidityProviderCacheSubCachePlugin {
	public:
		/// Creates a plugin around \a config, \a pcacheidCollector and \a pConfigHolder.
		explicit LiquidityProviderCacheSubCachePlugin(
			const CacheConfiguration& config,
			const std::shared_ptr<LiquidityProviderKeyCollector>& pKeyCollector,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
	};
}}
