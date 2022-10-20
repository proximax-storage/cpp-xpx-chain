/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ContractCacheDelta.h"
#include "ContractCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

    /// Cache composed of super contract information.
	using BasicContractCache = BasicCache<ContractCacheDescriptor, ContractCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of super contract information.
	class ContractCache : public SynchronizedCache<BasicContractCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Contract_v2)

	public:
		/// Creates a cache around \a config.
		explicit ContractCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SynchronizedCache<BasicContractCache>(BasicContractCache(config, std::move(pConfigHolder)))
		{}
	};
}}