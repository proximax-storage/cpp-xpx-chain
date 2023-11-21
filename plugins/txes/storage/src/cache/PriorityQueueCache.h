/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "PriorityQueueCacheDelta.h"
#include "PriorityQueueCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Cache composed of priority queue information.
	using BasicPriorityQueueCache = BasicCache<PriorityQueueCacheDescriptor, PriorityQueueCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of priority queue information.
	class PriorityQueueCache : public SynchronizedCache<BasicPriorityQueueCache> {
	public:
		DEFINE_CACHE_CONSTANTS(PriorityQueue)

	public:
		/// Creates a cache around \a config and \a pConfigHolder.
		explicit PriorityQueueCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SynchronizedCache<BasicPriorityQueueCache>(BasicPriorityQueueCache(config, std::move(pConfigHolder)))
		{}
	};
}}
