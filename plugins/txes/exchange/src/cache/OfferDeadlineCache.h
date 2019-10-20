/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferDeadlineCacheDelta.h"
#include "OfferDeadlineCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of offer deadline information.
	using BasicOfferDeadlineCache = BasicCache<OfferDeadlineCacheDescriptor, OfferDeadlineCacheTypes::BaseSets>;

	/// Synchronized cache composed of offer deadline information.
	class OfferDeadlineCache : public SynchronizedCache<BasicOfferDeadlineCache> {
	public:
		DEFINE_CACHE_CONSTANTS(OfferDeadline)

	public:
		/// Creates a cache around \a config.
		explicit OfferDeadlineCache(const CacheConfiguration& config) : SynchronizedCache<BasicOfferDeadlineCache>(BasicOfferDeadlineCache(config))
		{}
	};
}}
