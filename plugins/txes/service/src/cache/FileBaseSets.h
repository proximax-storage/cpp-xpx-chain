/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "FileCacheSerializers.h"
#include "FileCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicFilePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<FileCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<state::DriveFileKey>>;

	class FilePatriciaTree : public BasicFilePatriciaTree {
	public:
		using BasicFilePatriciaTree::BasicFilePatriciaTree;
		using Serializer = FileCacheDescriptor::Serializer;
	};

	using FileSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<FileCacheTypes::PrimaryTypes, FilePatriciaTree>;

	struct FileBaseSetDeltaPointers : public FileSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct FileBaseSets : public FileSingleSetCacheTypesAdapter::BaseSets<FileBaseSetDeltaPointers> {
		using FileSingleSetCacheTypesAdapter::BaseSets<FileBaseSetDeltaPointers>::BaseSets;
	};
}}
