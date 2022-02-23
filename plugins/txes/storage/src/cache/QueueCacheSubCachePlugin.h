/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "QueueCache.h"
#include "QueueCacheStorage.h"
#include "catapult/cache/SummaryAwareSubCachePluginAdapter.h"

namespace catapult { namespace cache {

	/// CacheStorage implementation for saving and loading summary network config cache data.
	class QueueCacheSummaryCacheStorage : public SummaryCacheStorage<QueueCache> {
	public:
		using SummaryCacheStorage<QueueCache>::SummaryCacheStorage;

	public:
		void saveAll(const CatapultCacheView& cacheView, io::OutputStream& output) const override;

		void saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const override;

		void loadAll(io::InputStream& input, size_t) override;
	};

	using BaseQueueCacheSubCachePlugin =
		SummaryAwareSubCachePluginAdapter<QueueCache, QueueCacheStorage, QueueCacheSummaryCacheStorage>;

	/// Specialized network config cache sub cache plugin.
	class QueueCacheSubCachePlugin : public BaseQueueCacheSubCachePlugin {
	public:
		/// Creates a plugin around \a config, \a pKeyCollector and \a pConfigHolder.
		explicit QueueCacheSubCachePlugin(
			const CacheConfiguration& config,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
	};
}}
