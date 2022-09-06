/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#pragma once
#include "GlobalStoreCacheSerializers.h"
#include "GlobalStoreCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicGlobalStorePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<GlobalStoreCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Hash256>>;

	class GlobalStorePatriciaTree : public BasicGlobalStorePatriciaTree {
	public:
		using BasicGlobalStorePatriciaTree::BasicGlobalStorePatriciaTree;
		using Serializer = GlobalStoreCacheDescriptor::Serializer;
	};

	using GlobalStoreSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<GlobalStoreCacheTypes::PrimaryTypes, GlobalStorePatriciaTree>;

	struct GlobalStoreBaseSetDeltaPointers : public GlobalStoreSingleSetCacheTypesAdapter::BaseSetDeltaPointers {};

	struct GlobalStoreBaseSets
			: public GlobalStoreSingleSetCacheTypesAdapter::BaseSets<GlobalStoreBaseSetDeltaPointers> {
		using GlobalStoreSingleSetCacheTypesAdapter::BaseSets<GlobalStoreBaseSetDeltaPointers>::BaseSets;
	};
}}
