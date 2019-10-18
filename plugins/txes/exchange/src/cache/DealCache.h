/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DealCacheDelta.h"
#include "DealCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of deal information.
	using BasicDealCache = BasicCache<DealCacheDescriptor, DealCacheTypes::BaseSets>;

	/// Synchronized cache composed of deal information.
	class DealCache : public SynchronizedCache<BasicDealCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Deal)

	public:
		/// Creates a cache around \a config.
		explicit DealCache(const CacheConfiguration& config) : SynchronizedCache<BasicDealCache>(BasicDealCache(config))
		{}
	};
}}
