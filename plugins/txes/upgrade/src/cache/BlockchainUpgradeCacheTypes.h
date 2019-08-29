/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/BlockchainUpgradeEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicBlockchainUpgradeCacheDelta;
		class BasicBlockchainUpgradeCacheView;
		struct BlockchainUpgradeBaseSetDeltaPointers;
		struct BlockchainUpgradeBaseSets;
		class BlockchainUpgradeCache;
		class BlockchainUpgradeCacheDelta;
		class BlockchainUpgradeCacheView;
		struct BlockchainUpgradeEntryPrimarySerializer;
		class BlockchainUpgradePatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a catapult upgrade cache.
	struct BlockchainUpgradeCacheDescriptor {
	public:
		static constexpr auto Name = "BlockchainUpgradeCache";

	public:
		// key value types
		using KeyType = Height;
		using ValueType = state::BlockchainUpgradeEntry;

		// cache types
		using CacheType = BlockchainUpgradeCache;
		using CacheDeltaType = BlockchainUpgradeCacheDelta;
		using CacheViewType = BlockchainUpgradeCacheView;

		using Serializer = BlockchainUpgradeEntryPrimarySerializer;
		using PatriciaTree = BlockchainUpgradePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.height();
		}
	};

	/// Catapult upgrade cache types.
	struct BlockchainUpgradeCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<BlockchainUpgradeCacheDescriptor, utils::BaseValueHasher<Height>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicBlockchainUpgradeCacheView, BasicBlockchainUpgradeCacheDelta, const Height&, state::BlockchainUpgradeEntry>;

		using BaseSetDeltaPointers = BlockchainUpgradeBaseSetDeltaPointers;
		using BaseSets = BlockchainUpgradeBaseSets;
	};
}}
