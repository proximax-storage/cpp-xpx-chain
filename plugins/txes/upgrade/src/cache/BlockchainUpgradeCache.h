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
	using BasicBlockchainUpgradeCache = BasicCache<BlockchainUpgradeCacheDescriptor, BlockchainUpgradeCacheTypes::BaseSets>;

	/// Synchronized cache composed of catapult upgrade information.
	class BlockchainUpgradeCache : public SynchronizedCache<BasicBlockchainUpgradeCache> {
	public:
		DEFINE_CACHE_CONSTANTS(BlockchainUpgrade)

	public:
		/// Creates a cache around \a config.
		explicit BlockchainUpgradeCache(const CacheConfiguration& config) : SynchronizedCache<BasicBlockchainUpgradeCache>(BasicBlockchainUpgradeCache(config))
		{}
	};
}}
