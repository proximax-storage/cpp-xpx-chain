/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LPCacheDelta.h"
#include "LPCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	using LPBasicCache = BasicCache<LPCacheDescriptor, LPCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Cache composed of drive information.
	class BasicLPCache : public LPBasicCache {
	public:
		/// Creates a cache.
		explicit BasicLPCache(
			const CacheConfiguration& config,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: LPBasicCache(config, std::move(pConfigHolder))
		{}

	public:
		/// Initializes the cache with \a keys.
		void init(const std::unordered_set<MosaicId, utils::BaseValueHasher<MosaicId>>& keys) {
			auto delta = createDelta(Height());
			for (const auto& key : keys)
				delta.insertKey(key);
			commit(delta);
		}

		void commit(const CacheDeltaType& delta) {
			LPBasicCache::commit(delta);
		}

	private:
	};

	/// Synchronized cache composed of drive information.
	class LPCache : public SynchronizedCacheWithInit<BasicLPCache> {
	public:
		DEFINE_CACHE_CONSTANTS(LP)

	public:
		/// Creates a cache around \a config.
		explicit LPCache(
			const CacheConfiguration& config,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SynchronizedCacheWithInit(BasicLPCache(config, pConfigHolder))
		{}
	};
}}
