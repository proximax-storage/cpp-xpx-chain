/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DealCacheSerializers.h"
#include "DealCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicDealPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<DealCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<utils::ShortHash>>;

	class DealPatriciaTree : public BasicDealPatriciaTree {
	public:
		using BasicDealPatriciaTree::BasicDealPatriciaTree;
		using Serializer = DealCacheDescriptor::Serializer;
	};

	using DealSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<DealCacheTypes::PrimaryTypes, DealPatriciaTree>;

	struct DealBaseSetDeltaPointers : public DealSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct DealBaseSets : public DealSingleSetCacheTypesAdapter::BaseSets<DealBaseSetDeltaPointers> {
		using DealSingleSetCacheTypesAdapter::BaseSets<DealBaseSetDeltaPointers>::BaseSets;
	};
}}
