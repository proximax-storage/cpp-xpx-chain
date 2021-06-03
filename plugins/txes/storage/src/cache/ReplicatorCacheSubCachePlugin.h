/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReplicatorCache.h"
#include "ReplicatorCacheStorage.h"
#include "catapult/cache/SummaryAwareSubCachePluginAdapter.h"

namespace catapult { namespace cache {

	/// CacheStorage implementation for saving and loading summary network config cache data.
	class ReplicatorCacheSummaryCacheStorage : public SummaryCacheStorage<ReplicatorCache> {
	public:
		using SummaryCacheStorage<ReplicatorCache>::SummaryCacheStorage;

	public:
		void saveAll(const CatapultCacheView& cacheView, io::OutputStream& output) const override;

		void saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const override;

		void loadAll(io::InputStream& input, size_t) override;
	};

	using BaseReplicatorCacheSubCachePlugin =
		SummaryAwareSubCachePluginAdapter<ReplicatorCache, ReplicatorCacheStorage, ReplicatorCacheSummaryCacheStorage>;

	/// Specialized network config cache sub cache plugin.
	class ReplicatorCacheSubCachePlugin : public BaseReplicatorCacheSubCachePlugin {
	public:
		/// Creates a plugin around \a config, \a pKeyCollector and \a pConfigHolder.
		explicit ReplicatorCacheSubCachePlugin(
			const CacheConfiguration& config,
			const std::shared_ptr<ReplicatorKeyCollector>& pKeyCollector,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
	};
}}
