/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NetworkConfigCacheDelta.h"
#include "NetworkConfigCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config/SupportedEntityVersions.h"
#include "catapult/config/BlockchainConfiguration.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Cache composed of network config information.
	using NetworkConfigBasicCache = BasicCache<NetworkConfigCacheDescriptor, NetworkConfigCacheTypes::BaseSets>;

	/// Cache composed of network config information.
	class BasicNetworkConfigCache : public NetworkConfigBasicCache {
	public:
		/// Creates a cache.
		explicit BasicNetworkConfigCache(
			const CacheConfiguration& config,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
				: NetworkConfigBasicCache(config)
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		/// Initializes the cache with \a heights.
		void init(const std::set<Height>& heights) {
			auto delta = createDelta(Height());
			for (const auto& height : heights)
				delta.insertHeight(height);
			commit(delta);
		}

		void commit(const CacheDeltaType& delta) {
			delta.updateConfigHolder(m_pConfigHolder);
			NetworkConfigBasicCache::commit(delta);
		}

	private:
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// Synchronized cache composed of network config information.
	class NetworkConfigCache : public SynchronizedCacheWithInit<BasicNetworkConfigCache> {
	public:
		DEFINE_CACHE_CONSTANTS(NetworkConfig)

	public:
		/// Creates a cache around \a config.
		explicit NetworkConfigCache(
			const CacheConfiguration& config,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
				: SynchronizedCacheWithInit<BasicNetworkConfigCache>(BasicNetworkConfigCache(config, pConfigHolder))
		{}
	};
}}
