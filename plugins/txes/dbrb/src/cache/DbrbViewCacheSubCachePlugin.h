/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbViewCache.h"
#include "DbrbViewCacheStorage.h"
#include "catapult/cache/SummaryAwareSubCachePluginAdapter.h"

namespace catapult { namespace cache {

	/// CacheStorage implementation for saving and loading summary DBRB view cache data.
	class DbrbViewCacheSummaryCacheStorage : public SummaryCacheStorage<DbrbViewCache> {
	public:
		using SummaryCacheStorage<DbrbViewCache>::SummaryCacheStorage;

	public:
		void saveAll(const CatapultCacheView& cacheView, io::OutputStream& output) const override;

		void saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const override;

		void loadAll(io::InputStream& input, size_t) override;
	};

	using BaseDbrbViewCacheSubCachePlugin = SummaryAwareSubCachePluginAdapter<DbrbViewCache, DbrbViewCacheStorage, DbrbViewCacheSummaryCacheStorage>;

	/// Specialized DBRB view cache sub cache plugin.
	class DbrbViewCacheSubCachePlugin : public BaseDbrbViewCacheSubCachePlugin {
	public:
		explicit DbrbViewCacheSubCachePlugin(
			const CacheConfiguration& config,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			std::shared_ptr<DbrbViewFetcherImpl> pDbrbViewFetcher);
	};
}}
