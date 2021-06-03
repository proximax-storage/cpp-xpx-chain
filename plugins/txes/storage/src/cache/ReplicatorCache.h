/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReplicatorCacheDelta.h"
#include "ReplicatorCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	using ReplicatorBasicCache = BasicCache<ReplicatorCacheDescriptor, ReplicatorCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Cache composed of replicator information.
	class BasicReplicatorCache : public ReplicatorBasicCache {
	public:
		/// Creates a cache.
		explicit BasicReplicatorCache(
			const CacheConfiguration& config,
			const std::shared_ptr<ReplicatorKeyCollector>& pKeyCollector,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReplicatorBasicCache(config, std::move(pConfigHolder))
				, m_pKeyCollector(pKeyCollector)
		{}

	public:
		/// Initializes the cache with \a keys.
		void init(const std::unordered_set<Key, utils::ArrayHasher<Key>>& keys) {
			auto delta = createDelta(Height());
			for (const auto& key : keys)
				delta.insertKey(key);
			commit(delta);
		}

		void commit(const CacheDeltaType& delta) {
			delta.updateKeyCollector(m_pKeyCollector);
			ReplicatorBasicCache::commit(delta);
		}

	private:
		std::shared_ptr<ReplicatorKeyCollector> m_pKeyCollector;
	};

	/// Synchronized cache composed of replicator information.
	class ReplicatorCache : public SynchronizedCacheWithInit<BasicReplicatorCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Replicator)

	public:
		/// Creates a cache around \a config.
		explicit ReplicatorCache(
			const CacheConfiguration& config,
			const std::shared_ptr<ReplicatorKeyCollector>& pKeyCollector,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SynchronizedCacheWithInit(BasicReplicatorCache(config, pKeyCollector, pConfigHolder))
		{}
	};
}}
