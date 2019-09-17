/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BlockchainUpgradeCacheSerializers.h"
#include "BlockchainUpgradeCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicBlockchainUpgradePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<BlockchainUpgradeCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<Height>>;

	class BlockchainUpgradePatriciaTree : public BasicBlockchainUpgradePatriciaTree {
	public:
		using BasicBlockchainUpgradePatriciaTree::BasicBlockchainUpgradePatriciaTree;
		using Serializer = BlockchainUpgradeCacheDescriptor::Serializer;
	};

	using BlockchainUpgradeSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<BlockchainUpgradeCacheTypes::PrimaryTypes, BlockchainUpgradePatriciaTree>;

	struct BlockchainUpgradeBaseSetDeltaPointers : public BlockchainUpgradeSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct BlockchainUpgradeBaseSets : public BlockchainUpgradeSingleSetCacheTypesAdapter::BaseSets<BlockchainUpgradeBaseSetDeltaPointers> {
		using BlockchainUpgradeSingleSetCacheTypesAdapter::BaseSets<BlockchainUpgradeBaseSetDeltaPointers>::BaseSets;
	};
}}
