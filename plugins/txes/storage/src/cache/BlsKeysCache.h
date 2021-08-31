/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BlsKeysCacheDelta.h"
#include "BlsKeysCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Cache composed of BLS keys information.
	using BasicBlsKeysCache = BasicCache<BlsKeysCacheDescriptor, BlsKeysCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of BLS keys information.
	class BlsKeysCache : public SynchronizedCache<BasicBlsKeysCache> {
	public:
		DEFINE_CACHE_CONSTANTS(BlsKeys)

	public:
		/// Creates a cache around \a config and \a pConfigHolder.
		explicit BlsKeysCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SynchronizedCache<BasicBlsKeysCache>(BasicBlsKeysCache(config, std::move(pConfigHolder)))
		{}
	};
}}
