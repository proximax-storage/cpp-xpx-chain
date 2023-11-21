/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaOfferGroupCacheDelta.h"
#include "SdaOfferGroupCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of SDA-SDA exchange information.
	using BasicSdaOfferGroupCache = BasicCache<SdaOfferGroupCacheDescriptor, SdaOfferGroupCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of SDA-SDA exchange information.
	class SdaOfferGroupCache : public SynchronizedCache<BasicSdaOfferGroupCache> {
	public:
		DEFINE_CACHE_CONSTANTS(SdaOfferGroup)

	public:
		/// Creates a cache around \a config.
		explicit SdaOfferGroupCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
		: SynchronizedCache<BasicSdaOfferGroupCache>(BasicSdaOfferGroupCache(config, std::move(pConfigHolder)))
		{}
	};
}}
