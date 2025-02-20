/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BlockchainUpgradeCacheDelta.h"
#include "BlockchainUpgradeCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of catapult upgrade information.
	using BlockchainUpgradeBasicCache = BasicCache<BlockchainUpgradeCacheDescriptor, BlockchainUpgradeCacheTypes::BaseSets>;

	/// Cache composed of network config information.
	class BasicBlockchainUpgradeCache : public BlockchainUpgradeBasicCache {
	public:
		/// Creates a cache.
		explicit BasicBlockchainUpgradeCache(
			const CacheConfiguration& config,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BlockchainUpgradeBasicCache(config)
				, m_pConfigHolder(std::move(pConfigHolder))
		{}

	public:
		void init() {
			auto view = createView(Height());
			auto entries = view.getAll();
			for (const auto& entry : entries)
				m_pConfigHolder->InsertBlockchainVersion(entry.height(), entry.blockChainVersion());
		}

		void commit(const CacheDeltaType& delta) {
			for (const auto& pEntry : delta.addedElements())
				m_pConfigHolder->InsertBlockchainVersion(pEntry->height(), pEntry->blockChainVersion());
			for (const auto& pEntry : delta.modifiedElements())
				m_pConfigHolder->InsertBlockchainVersion(pEntry->height(), pEntry->blockChainVersion());
			for (const auto& pEntry : delta.removedElements())
				m_pConfigHolder->RemoveBlockchainVersion(pEntry->height());

			BlockchainUpgradeBasicCache::commit(delta);
		}

	private:
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// Synchronized cache composed of catapult upgrade information.
	class BlockchainUpgradeCache : public SynchronizedCacheWithInit<BasicBlockchainUpgradeCache> {
	public:
		DEFINE_CACHE_CONSTANTS(BlockchainUpgrade)

	public:
		/// Creates a cache around \a config.
		explicit BlockchainUpgradeCache(
			const CacheConfiguration& config,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SynchronizedCacheWithInit<BasicBlockchainUpgradeCache>(BasicBlockchainUpgradeCache(config, std::move(pConfigHolder)))
		{}
	};
}}
