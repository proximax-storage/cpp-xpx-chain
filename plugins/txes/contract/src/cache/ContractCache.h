/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ContractCacheDelta.h"
#include "ContractCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of contract information.
	using BasicContractCache = BasicCache<ContractCacheDescriptor, ContractCacheTypes::BaseSets>;

	/// Synchronized cache composed of contract information.
	class ContractCache : public SynchronizedCache<BasicContractCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Contract)

	public:
		/// Creates a cache around \a config.
		explicit ContractCache(const CacheConfiguration& config) : SynchronizedCache<BasicContractCache>(BasicContractCache(config))
		{}
	};
}}
