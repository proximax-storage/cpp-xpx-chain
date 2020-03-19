/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NetworkConfigCache.h"
#include "NetworkConfigCacheStorage.h"
#include "catapult/cache/SummaryAwareSubCachePluginAdapter.h"

namespace catapult { namespace cache {

	/// CacheStorage implementation for saving and loading summary network config cache data.
	class NetworkConfigCacheSummaryCacheStorage : public SummaryCacheStorage<NetworkConfigCache> {
	public:
		using SummaryCacheStorage<NetworkConfigCache>::SummaryCacheStorage;

	public:
		void saveAll(const CatapultCacheView& cacheView, io::OutputStream& output) const override;

		void saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const override;

		void loadAll(io::InputStream& input, size_t) override;
	};

	using BaseNetworkConfigCacheSubCachePlugin =
		SummaryAwareSubCachePluginAdapter<NetworkConfigCache, NetworkConfigCacheStorage, NetworkConfigCacheSummaryCacheStorage>;

	/// Specialized network config cache sub cache plugin.
	class NetworkConfigCacheSubCachePlugin : public BaseNetworkConfigCacheSubCachePlugin {
	public:
		/// Creates a plugin around \a config and \a options.
		explicit NetworkConfigCacheSubCachePlugin(const CacheConfiguration& config,
		                                          const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
	};
}}
