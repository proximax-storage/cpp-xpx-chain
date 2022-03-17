/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include "catapult/cache/BasicCache.h"
#include "LockFundCacheView.h"
#include "LockFundCacheDelta.h"
#include "src/state/LockFundRecordGroup.h"
#include "LockFundCacheTypes.h"

namespace catapult { namespace cache {

	/// Cache composed of hash lock info information.
	using BasicLockFundCache = BasicCache<LockFundCacheDescriptor, LockFundCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of hash lock info information.
	class LockFundCache : public SynchronizedCache<BasicLockFundCache> {
	public:
		DEFINE_CACHE_CONSTANTS(LockFund)

		using CacheValueTypes = std::tuple<typename LockFundCache::CacheValueType, state::LockFundRecordGroup<state::LockFundKeyIndexDescriptor>>;
	public:
		/// Creates a cache around \a config.
		explicit LockFundCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SynchronizedCache<BasicLockFundCache>(BasicLockFundCache(config, std::move(pConfigHolder)))
		{}


	};
}}
