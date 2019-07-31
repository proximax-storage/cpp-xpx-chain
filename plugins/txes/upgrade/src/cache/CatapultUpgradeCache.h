/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultUpgradeCacheDelta.h"
#include "CatapultUpgradeCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of catapult upgrade information.
	using BasicCatapultUpgradeCache = BasicCache<CatapultUpgradeCacheDescriptor, CatapultUpgradeCacheTypes::BaseSets>;

	/// Synchronized cache composed of catapult upgrade information.
	class CatapultUpgradeCache : public SynchronizedCache<BasicCatapultUpgradeCache> {
	public:
		DEFINE_CACHE_CONSTANTS(CatapultUpgrade)

	public:
		/// Creates a cache around \a config.
		explicit CatapultUpgradeCache(const CacheConfiguration& config) : SynchronizedCache<BasicCatapultUpgradeCache>(BasicCatapultUpgradeCache(config))
		{}
	};
}}
