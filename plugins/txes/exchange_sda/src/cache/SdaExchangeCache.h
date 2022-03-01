/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaExchangeCacheDelta.h"
#include "SdaExchangeCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of SDA-SDA exchange information.
	using BasicSdaExchangeCache = BasicCache<SdaExchangeCacheDescriptor, SdaExchangeCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of SDA-SDA exchange information.
	class SdaExchangeCache : public SynchronizedCache<BasicSdaExchangeCache> {
	public:
		DEFINE_CACHE_CONSTANTS(ExchangeSda)

	public:
		/// Creates a cache around \a config.
		explicit SdaExchangeCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
		: SynchronizedCache<BasicSdaExchangeCache>(BasicSdaExchangeCache(config, std::move(pConfigHolder)))
		{}
	};
}}
