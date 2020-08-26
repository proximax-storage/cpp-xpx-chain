/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeCacheSerializers.h"
#include "CommitteeCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicCommitteePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<CommitteeCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class CommitteePatriciaTree : public BasicCommitteePatriciaTree {
	public:
		using BasicCommitteePatriciaTree::BasicCommitteePatriciaTree;
		using Serializer = CommitteeCacheDescriptor::Serializer;
	};

	using CommitteeSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<CommitteeCacheTypes::PrimaryTypes, CommitteePatriciaTree>;

	struct CommitteeBaseSetDeltaPointers : public CommitteeSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct CommitteeBaseSets : public CommitteeSingleSetCacheTypesAdapter::BaseSets<CommitteeBaseSetDeltaPointers> {
		using CommitteeSingleSetCacheTypesAdapter::BaseSets<CommitteeBaseSetDeltaPointers>::BaseSets;
	};
}}
