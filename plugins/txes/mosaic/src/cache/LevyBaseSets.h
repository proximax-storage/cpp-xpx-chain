/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "LevyCacheSerializers.h"
#include "LevyCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {
		
	using BasicLevyPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<LevyCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;
		
	class LevyPatriciaTree : public BasicLevyPatriciaTree {
	public:
		using BasicLevyPatriciaTree::BasicLevyPatriciaTree;
		using Serializer = LevyCacheDescriptor::Serializer;
	};
		
	using LevySingleSetCacheTypesAdapter =
	SingleSetAndPatriciaTreeCacheTypesAdapter<LevyCacheTypes::PrimaryTypes, LevyPatriciaTree>;
		
	struct LevyBaseSetDeltaPointers : public LevySingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};
		
	struct LevyBaseSets : public LevySingleSetCacheTypesAdapter::BaseSets<LevyBaseSetDeltaPointers> {
		using LevySingleSetCacheTypesAdapter::BaseSets<LevyBaseSetDeltaPointers>::BaseSets;
	};
}}

