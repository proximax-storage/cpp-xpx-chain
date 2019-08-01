/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultConfigCacheDelta.h"
#include "CatapultConfigCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of catapult config information.
	using CatapultConfigBasicCache = BasicCache<CatapultConfigCacheDescriptor, CatapultConfigCacheTypes::BaseSets>;

	/// Cache composed of catapult config information.
	class BasicCatapultConfigCache : public CatapultConfigBasicCache {
	public:
		/// Creates a cache.
		explicit BasicCatapultConfigCache(const CacheConfiguration& config)
				: CatapultConfigBasicCache(config)
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

	/// Synchronized cache composed of catapult config information.
	class CatapultConfigCache : public SynchronizedCacheWithInit<BasicCatapultConfigCache> {
	public:
		DEFINE_CACHE_CONSTANTS(CatapultConfig)

	public:
		/// Creates a cache around \a config.
		explicit CatapultConfigCache(const CacheConfiguration& config) : SynchronizedCacheWithInit<BasicCatapultConfigCache>(BasicCatapultConfigCache(config))
		{}
	};
}}
