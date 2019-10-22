/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferCacheDelta.h"
#include "OfferCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of offer information.
	using BasicOfferCache = BasicCache<OfferCacheDescriptor, OfferCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of offer information.
	class OfferCache : public SynchronizedCache<BasicOfferCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Offer)

	public:
		/// Creates a cache around \a config.
		explicit OfferCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
		: SynchronizedCache<BasicOfferCache>(BasicOfferCache(config, std::move(pConfigHolder)))
		{}
	};
}}
