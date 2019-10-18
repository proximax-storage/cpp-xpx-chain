/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BuyOfferCacheDelta.h"
#include "BuyOfferCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of buy offer information.
	using BasicBuyOfferCache = BasicCache<BuyOfferCacheDescriptor, BuyOfferCacheTypes::BaseSets>;

	/// Synchronized cache composed of buy offer information.
	class BuyOfferCache : public SynchronizedCache<BasicBuyOfferCache> {
	public:
		DEFINE_CACHE_CONSTANTS(BuyOffer)

	public:
		/// Creates a cache around \a config.
		explicit BuyOfferCache(const CacheConfiguration& config) : SynchronizedCache<BasicBuyOfferCache>(BasicBuyOfferCache(config))
		{}
	};
}}
