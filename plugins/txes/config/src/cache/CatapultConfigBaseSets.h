/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultConfigCacheSerializers.h"
#include "CatapultConfigCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicCatapultConfigPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<CatapultConfigCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<Height>>;

	class CatapultConfigPatriciaTree : public BasicCatapultConfigPatriciaTree {
	public:
		using BasicCatapultConfigPatriciaTree::BasicCatapultConfigPatriciaTree;
		using Serializer = CatapultConfigCacheDescriptor::Serializer;
	};

	using CatapultConfigSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<CatapultConfigCacheTypes::PrimaryTypes, CatapultConfigPatriciaTree>;

	struct CatapultConfigBaseSetDeltaPointers : public CatapultConfigSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct CatapultConfigBaseSets : public CatapultConfigSingleSetCacheTypesAdapter::BaseSets<CatapultConfigBaseSetDeltaPointers> {
		using CatapultConfigSingleSetCacheTypesAdapter::BaseSets<CatapultConfigBaseSetDeltaPointers>::BaseSets;
	};
}}
