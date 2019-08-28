/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NetworkConfigCacheDelta.h"
#include "NetworkConfigCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of network config information.
	using NetworkConfigBasicCache = BasicCache<NetworkConfigCacheDescriptor, NetworkConfigCacheTypes::BaseSets>;

	/// Cache composed of network config information.
	class BasicNetworkConfigCache : public NetworkConfigBasicCache {
	public:
		/// Creates a cache.
		explicit BasicNetworkConfigCache(const CacheConfiguration& config)
				: NetworkConfigBasicCache(config)
		{}

	public:
		/// Initializes the cache with \a heights.
		void init(const std::set<Height>& heights) {
			auto delta = createDelta(Height());
			for (const auto& height : heights)
				delta.insertHeight(height);
			commit(delta);
		}
	};

	/// Synchronized cache composed of network config information.
	class NetworkConfigCache : public SynchronizedCacheWithInit<BasicNetworkConfigCache> {
	public:
		DEFINE_CACHE_CONSTANTS(NetworkConfig)

	public:
		/// Creates a cache around \a config.
		explicit NetworkConfigCache(const CacheConfiguration& config) : SynchronizedCacheWithInit<BasicNetworkConfigCache>(BasicNetworkConfigCache(config))
		{}
	};
}}
