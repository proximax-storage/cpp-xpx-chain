/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#pragma once
#include "GlobalStoreCacheDelta.h"
#include "GlobalStoreCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/model/NetworkInfo.h"

namespace catapult { namespace cache {

	/// Cache composed of global store information.
	using BasicGlobalStoreCache = BasicCache<
		GlobalStoreCacheDescriptor,
		GlobalStoreCacheTypes::BaseSets,
		std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of global store information.
	class GlobalStoreCache : public SynchronizedCache<BasicGlobalStoreCache> {
	public:
		DEFINE_CACHE_CONSTANTS(GlobalStore)

	public:
		/// Creates a cache around \a config and \a networkIdentifier.
		GlobalStoreCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SynchronizedCache<BasicGlobalStoreCache>(BasicGlobalStoreCache(config, std::move(pConfigHolder)))
		{}
	};
}}
