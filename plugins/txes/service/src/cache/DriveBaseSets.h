/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveCacheSerializers.h"
#include "DriveCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicDrivePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<DriveCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class DrivePatriciaTree : public BasicDrivePatriciaTree {
	public:
		using BasicDrivePatriciaTree::BasicDrivePatriciaTree;
		using Serializer = DriveCacheDescriptor::Serializer;
	};

	using DriveSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<DriveCacheTypes::PrimaryTypes, DrivePatriciaTree>;

	struct DriveBaseSetDeltaPointers : public DriveSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct DriveBaseSets : public DriveSingleSetCacheTypesAdapter::BaseSets<DriveBaseSetDeltaPointers> {
		using DriveSingleSetCacheTypesAdapter::BaseSets<DriveBaseSetDeltaPointers>::BaseSets;
	};
}}
