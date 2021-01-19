/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeCache.h"
#include "CommitteeCacheStorage.h"
#include "catapult/cache/SummaryAwareSubCachePluginAdapter.h"

namespace catapult { namespace cache {

	/// CacheStorage implementation for saving and loading summary network config cache data.
	class CommitteeCacheSummaryCacheStorage : public SummaryCacheStorage<CommitteeCache> {
	public:
		using SummaryCacheStorage<CommitteeCache>::SummaryCacheStorage;

	public:
		void saveAll(const CatapultCacheView& cacheView, io::OutputStream& output) const override;

		void saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const override;

		void loadAll(io::InputStream& input, size_t) override;
	};

	using BaseCommitteeCacheSubCachePlugin =
		SummaryAwareSubCachePluginAdapter<CommitteeCache, CommitteeCacheStorage, CommitteeCacheSummaryCacheStorage>;

	/// Specialized network config cache sub cache plugin.
	class CommitteeCacheSubCachePlugin : public BaseCommitteeCacheSubCachePlugin {
	public:
		/// Creates a plugin around \a config, \a pAccountCollector and \a pConfigHolder.
		explicit CommitteeCacheSubCachePlugin(
			const CacheConfiguration& config,
			const std::shared_ptr<CommitteeAccountCollector>& pAccountCollector,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
	};
}}
