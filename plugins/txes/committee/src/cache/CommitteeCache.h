/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeCacheDelta.h"
#include "CommitteeCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	using CommitteeBasicCache = BasicCache<CommitteeCacheDescriptor, CommitteeCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Cache composed of committee information.
	class BasicCommitteeCache : public CommitteeBasicCache {
	public:
		/// Creates a cache.
		explicit BasicCommitteeCache(
			const CacheConfiguration& config,
			const std::shared_ptr<CommitteeAccountCollector>& pAccountCollector,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: CommitteeBasicCache(config, std::move(pConfigHolder))
				, m_pAccountCollector(pAccountCollector)
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
			delta.updateAccountCollector(m_pAccountCollector);
			CommitteeBasicCache::commit(delta);
		}

	private:
		std::shared_ptr<CommitteeAccountCollector> m_pAccountCollector;
	};

	/// Synchronized cache composed of committee information.
	class CommitteeCache : public SynchronizedCacheWithInit<BasicCommitteeCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Committee)

	public:
		/// Creates a cache around \a config.
		explicit CommitteeCache(
			const CacheConfiguration& config,
			const std::shared_ptr<CommitteeAccountCollector>& pAccountCollector,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SynchronizedCacheWithInit(BasicCommitteeCache(config, pAccountCollector, pConfigHolder))
		{}
	};
}}
