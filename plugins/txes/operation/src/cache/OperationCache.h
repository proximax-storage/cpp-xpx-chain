/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationCacheDelta.h"
#include "OperationCacheStorage.h"
#include "OperationCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Cache composed of operation information.
	using BasicOperationCache = BasicCache<OperationCacheDescriptor, OperationCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of operation information.
	class OperationCache : public SynchronizedCache<BasicOperationCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Operation)

	public:
		/// Creates a cache around \a config and \a pConfigHolder.
		explicit OperationCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SynchronizedCache<BasicOperationCache>(BasicOperationCache(config, std::move(pConfigHolder)))
		{}
	};
}}
