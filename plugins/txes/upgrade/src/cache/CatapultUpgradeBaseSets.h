/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultUpgradeCacheSerializers.h"
#include "CatapultUpgradeCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicCatapultUpgradePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<CatapultUpgradeCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<Height>>;

	class CatapultUpgradePatriciaTree : public BasicCatapultUpgradePatriciaTree {
	public:
		using BasicCatapultUpgradePatriciaTree::BasicCatapultUpgradePatriciaTree;
		using Serializer = CatapultUpgradeCacheDescriptor::Serializer;
	};

	using CatapultUpgradeSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<CatapultUpgradeCacheTypes::PrimaryTypes, CatapultUpgradePatriciaTree>;

	struct CatapultUpgradeBaseSetDeltaPointers : public CatapultUpgradeSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct CatapultUpgradeBaseSets : public CatapultUpgradeSingleSetCacheTypesAdapter::BaseSets<CatapultUpgradeBaseSetDeltaPointers> {
		using CatapultUpgradeSingleSetCacheTypesAdapter::BaseSets<CatapultUpgradeBaseSetDeltaPointers>::BaseSets;
	};
}}
