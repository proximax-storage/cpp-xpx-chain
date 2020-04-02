/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractCacheDelta.h"
#include "SuperContractCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Cache composed of super contract information.
	using BasicSuperContractCache = BasicCache<SuperContractCacheDescriptor, SuperContractCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of super contract information.
	class SuperContractCache : public SynchronizedCache<BasicSuperContractCache> {
	public:
		DEFINE_CACHE_CONSTANTS(SuperContract)

	public:
		/// Creates a cache around \a config.
		explicit SuperContractCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SynchronizedCache<BasicSuperContractCache>(BasicSuperContractCache(config, std::move(pConfigHolder)))
		{}
	};
}}
