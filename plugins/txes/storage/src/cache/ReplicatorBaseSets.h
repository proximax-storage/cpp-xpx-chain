/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReplicatorCacheSerializers.h"
#include "ReplicatorCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicReplicatorPatriciaTree = tree::BasePatriciaTree<
			SerializerHashedKeyEncoder<ReplicatorCacheDescriptor::Serializer>,
			PatriciaTreeRdbDataSource,
			utils::ArrayHasher<Key>>;

	class ReplicatorPatriciaTree : public BasicReplicatorPatriciaTree {
	public:
		using BasicReplicatorPatriciaTree::BasicReplicatorPatriciaTree;
		using Serializer = ReplicatorCacheDescriptor::Serializer;
	};

	using ReplicatorSingleSetCacheTypesAdapter =
			SingleSetAndPatriciaTreeCacheTypesAdapter<ReplicatorCacheTypes::PrimaryTypes, ReplicatorPatriciaTree>;

	struct ReplicatorBaseSetDeltaPointers : public ReplicatorSingleSetCacheTypesAdapter::BaseSetDeltaPointers {};

	struct ReplicatorBaseSets : public ReplicatorSingleSetCacheTypesAdapter::BaseSets<ReplicatorBaseSetDeltaPointers> {
		using ReplicatorSingleSetCacheTypesAdapter::BaseSets<ReplicatorBaseSetDeltaPointers>::BaseSets;
	};
}}
