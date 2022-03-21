/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "QueueCacheDelta.h"
#include "QueueCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	using QueueBasicCache = BasicCache<QueueCacheDescriptor, QueueCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Cache composed of drive information.
	class BasicQueueCache : public QueueBasicCache {
	public:
		/// Creates a cache.
		explicit BasicQueueCache(
			const CacheConfiguration& config,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: QueueBasicCache(config, std::move(pConfigHolder))
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
			QueueBasicCache::commit(delta);
		}

	private:
		std::shared_ptr<DriveKeyCollector> m_pKeyCollector;
	};

	/// Synchronized cache composed of drive information.
	class QueueCache : public SynchronizedCacheWithInit<BasicQueueCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Queue)

	public:
		/// Creates a cache around \a config.
		explicit QueueCache(
			const CacheConfiguration& config,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SynchronizedCacheWithInit(BasicQueueCache(config, pConfigHolder))
		{}
	};
}}
