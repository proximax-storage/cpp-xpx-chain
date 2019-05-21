/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReputationCacheDelta.h"
#include "ReputationCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of reputation information.
	using BasicReputationCache = BasicCache<ReputationCacheDescriptor, ReputationCacheTypes::BaseSets>;

	/// Synchronized cache composed of reputation information.
	class ReputationCache : public SynchronizedCache<BasicReputationCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Reputation)

	public:
		/// Creates a cache around \a config.
		explicit ReputationCache(const CacheConfiguration& config) : SynchronizedCache<BasicReputationCache>(BasicReputationCache(config))
		{}
	};
}}
