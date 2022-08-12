/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReplicatorCacheDelta.h"
#include "ReplicatorCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Cache composed of drive information.
	using BasicReplicatorCache = BasicCache<ReplicatorCacheDescriptor, ReplicatorCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of drive information.
	class ReplicatorCache : public SynchronizedCache<BasicReplicatorCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Replicator)

	public:
		/// Creates a cache around \a config and \a pConfigHolder.
		explicit ReplicatorCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
		: SynchronizedCache<BasicReplicatorCache>(BasicReplicatorCache(config, std::move(pConfigHolder)))
		{}
	};
}}
