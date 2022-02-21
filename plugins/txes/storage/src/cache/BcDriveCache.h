/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BcDriveCacheDelta.h"
#include "BcDriveCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	using BcDriveBasicCache = BasicCache<BcDriveCacheDescriptor, BcDriveCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Cache composed of drive information.
	class BasicBcDriveCache : public BcDriveBasicCache {
	public:
		/// Creates a cache.
		explicit BasicBcDriveCache(
			const CacheConfiguration& config,
			const std::shared_ptr<DriveKeyCollector>& pKeyCollector,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BcDriveBasicCache(config, std::move(pConfigHolder))
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
			BcDriveBasicCache::commit(delta);
		}

	private:
		std::shared_ptr<DriveKeyCollector> m_pKeyCollector;
	};

	/// Synchronized cache composed of drive information.
	class BcDriveCache : public SynchronizedCacheWithInit<BasicBcDriveCache> {
	public:
		DEFINE_CACHE_CONSTANTS(BcDrive)

	public:
		/// Creates a cache around \a config.
		explicit BcDriveCache(
			const CacheConfiguration& config,
			const std::shared_ptr<DriveKeyCollector>& pKeyCollector,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SynchronizedCacheWithInit(BasicBcDriveCache(config, pKeyCollector, pConfigHolder))
		{}
	};
}}
