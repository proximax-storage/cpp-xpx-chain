/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LPCache.h"
#include "LPCacheStorage.h"
#include "catapult/cache/SummaryAwareSubCachePluginAdapter.h"

namespace catapult { namespace cache {

	/// CacheStorage implementation for saving and loading summary network config cache data.
	class LPCacheSummaryCacheStorage : public SummaryCacheStorage<LPCache> {
	public:
		using SummaryCacheStorage<LPCache>::SummaryCacheStorage;

	public:
		void saveAll(const CatapultCacheView& cacheView, io::OutputStream& output) const override;

		void saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const override;

		void loadAll(io::InputStream& input, size_t) override;
	};

	using BaseLPCacheSubCachePlugin =
		SummaryAwareSubCachePluginAdapter<LPCache, LPCacheStorage, LPCacheSummaryCacheStorage>;

	/// Specialized network config cache sub cache plugin.
	class LPCacheSubCachePlugin : public BaseLPCacheSubCachePlugin {
	public:
		/// Creates a plugin around \a config, \a pKeyCollector and \a pConfigHolder.
		explicit LPCacheSubCachePlugin(
			const CacheConfiguration& config,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
	};
}}
