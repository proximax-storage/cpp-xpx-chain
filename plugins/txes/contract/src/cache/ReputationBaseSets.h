/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReputationCacheSerializers.h"
#include "ReputationCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicReputationPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<ReputationCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class ReputationPatriciaTree : public BasicReputationPatriciaTree {
	public:
		using BasicReputationPatriciaTree::BasicReputationPatriciaTree;
		using Serializer = ReputationCacheDescriptor::Serializer;
	};

	using ReputationSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<ReputationCacheTypes::PrimaryTypes, ReputationPatriciaTree>;

	struct ReputationBaseSetDeltaPointers : public ReputationSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct ReputationBaseSets : public ReputationSingleSetCacheTypesAdapter::BaseSets<ReputationBaseSetDeltaPointers> {
		using ReputationSingleSetCacheTypesAdapter::BaseSets<ReputationBaseSetDeltaPointers>::BaseSets;
	};
}}
