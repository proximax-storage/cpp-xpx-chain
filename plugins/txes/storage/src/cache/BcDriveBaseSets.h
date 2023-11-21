/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BcDriveCacheSerializers.h"
#include "BcDriveCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicBcDrivePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<BcDriveCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class BcDrivePatriciaTree : public BasicBcDrivePatriciaTree {
	public:
		using BasicBcDrivePatriciaTree::BasicBcDrivePatriciaTree;
		using Serializer = BcDriveCacheDescriptor::Serializer;
	};

	using BcDriveSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<BcDriveCacheTypes::PrimaryTypes, BcDrivePatriciaTree>;

	struct BcDriveBaseSetDeltaPointers : public BcDriveSingleSetCacheTypesAdapter::BaseSetDeltaPointers {};

	struct BcDriveBaseSets : public BcDriveSingleSetCacheTypesAdapter::BaseSets<BcDriveBaseSetDeltaPointers> {
		using BcDriveSingleSetCacheTypesAdapter::BaseSets<BcDriveBaseSetDeltaPointers>::BaseSets;
	};
}}
