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
	using BasicCatapultConfigCache = BasicCache<CatapultConfigCacheDescriptor, CatapultConfigCacheTypes::BaseSets>;

	/// Synchronized cache composed of catapult config information.
	class CatapultConfigCache : public SynchronizedCache<BasicCatapultConfigCache> {
	public:
		DEFINE_CACHE_CONSTANTS(CatapultConfig)

	public:
		/// Creates a cache around \a config.
		explicit CatapultConfigCache(const CacheConfiguration& config) : SynchronizedCache<BasicCatapultConfigCache>(BasicCatapultConfigCache(config))
		{}
	};
}}
