/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BcDriveCache.h"
#include "BcDriveCacheStorage.h"
#include "catapult/cache/SummaryAwareSubCachePluginAdapter.h"

namespace catapult { namespace cache {

	/// CacheStorage implementation for saving and loading summary network config cache data.
	class BcDriveCacheSummaryCacheStorage : public SummaryCacheStorage<BcDriveCache> {
	public:
		using SummaryCacheStorage<BcDriveCache>::SummaryCacheStorage;

	public:
		void saveAll(const CatapultCacheView& cacheView, io::OutputStream& output) const override;

		void saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const override;

		void loadAll(io::InputStream& input, size_t) override;
	};

	using BaseBcDriveCacheSubCachePlugin =
		SummaryAwareSubCachePluginAdapter<BcDriveCache, BcDriveCacheStorage, BcDriveCacheSummaryCacheStorage>;

	/// Specialized network config cache sub cache plugin.
	class BcDriveCacheSubCachePlugin : public BaseBcDriveCacheSubCachePlugin {
	public:
		/// Creates a plugin around \a config, \a pKeyCollector and \a pConfigHolder.
		explicit BcDriveCacheSubCachePlugin(
			const CacheConfiguration& config,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
	};
}}
