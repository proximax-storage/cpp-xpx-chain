/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BootKeyReplicatorCacheSerializers.h"
#include "BootKeyReplicatorCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicBootKeyReplicatorPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<BootKeyReplicatorCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class BootKeyReplicatorPatriciaTree : public BasicBootKeyReplicatorPatriciaTree {
	public:
		using BasicBootKeyReplicatorPatriciaTree::BasicBootKeyReplicatorPatriciaTree;
		using Serializer = BootKeyReplicatorCacheDescriptor::Serializer;
	};

	using BootKeyReplicatorSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<BootKeyReplicatorCacheTypes::PrimaryTypes, BootKeyReplicatorPatriciaTree>;

	struct BootKeyReplicatorBaseSetDeltaPointers : public BootKeyReplicatorSingleSetCacheTypesAdapter::BaseSetDeltaPointers {};

	struct BootKeyReplicatorBaseSets : public BootKeyReplicatorSingleSetCacheTypesAdapter::BaseSets<BootKeyReplicatorBaseSetDeltaPointers> {
		using BootKeyReplicatorSingleSetCacheTypesAdapter::BaseSets<BootKeyReplicatorBaseSetDeltaPointers>::BaseSets;
	};
}}
