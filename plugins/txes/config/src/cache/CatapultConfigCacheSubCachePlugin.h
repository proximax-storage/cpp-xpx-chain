/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultConfigCache.h"
#include "CatapultConfigCacheStorage.h"
#include "catapult/cache/SummaryAwareSubCachePluginAdapter.h"

namespace catapult { namespace cache {

	/// CacheStorage implementation for saving and loading summary catapult config cache data.
	class CatapultConfigCacheSummaryCacheStorage : public SummaryCacheStorage<CatapultConfigCache> {
	public:
		using SummaryCacheStorage<CatapultConfigCache>::SummaryCacheStorage;

	public:
		void saveAll(const CatapultCacheView& cacheView, io::OutputStream& output) const override;

		void saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const override;

		void loadAll(io::InputStream& input, size_t) override;
	};

	using BaseCatapultConfigCacheSubCachePlugin =
		SummaryAwareSubCachePluginAdapter<CatapultConfigCache, CatapultConfigCacheStorage, CatapultConfigCacheSummaryCacheStorage>;

	/// Specialized catapult config cache sub cache plugin.
	class CatapultConfigCacheSubCachePlugin : public BaseCatapultConfigCacheSubCachePlugin {
	public:
		/// Creates a plugin around \a config and \a options.
		explicit CatapultConfigCacheSubCachePlugin(const CacheConfiguration& config);
	};
}}
