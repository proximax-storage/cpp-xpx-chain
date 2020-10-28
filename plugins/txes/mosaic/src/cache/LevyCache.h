/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include "LevyCacheDelta.h"
#include "LevyCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {
		
	/// Cache composed of levy information.
	using BasicLevyCache = BasicCache<LevyCacheDescriptor, LevyCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;
		
	/// Synchronized cache composed of levy information.
	class LevyCache : public SynchronizedCache<BasicLevyCache> {
	public:
		DEFINE_CACHE_CONSTANTS(MosaicLevy)
		
	public:
		/// Creates a cache around \a config.
		explicit LevyCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SynchronizedCache<BasicLevyCache>(BasicLevyCache(config, std::move(pConfigHolder)))
		{}
	};
}}
