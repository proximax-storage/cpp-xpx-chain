/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BootKeyReplicatorCacheDelta.h"
#include "BootKeyReplicatorCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Cache composed of drive information.
	using BasicBootKeyReplicatorCache = BasicCache<BootKeyReplicatorCacheDescriptor, BootKeyReplicatorCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of drive information.
	class BootKeyReplicatorCache : public SynchronizedCache<BasicBootKeyReplicatorCache> {
	public:
		DEFINE_CACHE_CONSTANTS(BootKeyReplicator)

	public:
		/// Creates a cache around \a config and \a pConfigHolder.
		explicit BootKeyReplicatorCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
		: SynchronizedCache<BasicBootKeyReplicatorCache>(BasicBootKeyReplicatorCache(config, std::move(pConfigHolder)))
		{}
	};
}}
