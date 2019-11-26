/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeCacheDelta.h"
#include "ExchangeCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of exchange information.
	using BasicExchangeCache = BasicCache<ExchangeCacheDescriptor, ExchangeCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of exchange information.
	class ExchangeCache : public SynchronizedCache<BasicExchangeCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Exchange)

	public:
		/// Creates a cache around \a config.
		explicit ExchangeCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
		: SynchronizedCache<BasicExchangeCache>(BasicExchangeCache(config, std::move(pConfigHolder)))
		{}
	};
}}
