/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SellOfferCacheDelta.h"
#include "SellOfferCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of sell offer information.
	using BasicSellOfferCache = BasicCache<SellOfferCacheDescriptor, SellOfferCacheTypes::BaseSets>;

	/// Synchronized cache composed of sell offer information.
	class SellOfferCache : public SynchronizedCache<BasicSellOfferCache> {
	public:
		DEFINE_CACHE_CONSTANTS(SellOffer)

	public:
		/// Creates a cache around \a config.
		explicit SellOfferCache(const CacheConfiguration& config) : SynchronizedCache<BasicSellOfferCache>(BasicSellOfferCache(config))
		{}
	};
}}
