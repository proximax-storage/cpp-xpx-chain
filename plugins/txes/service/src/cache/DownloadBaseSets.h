/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadCacheSerializers.h"
#include "DownloadCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicDownloadPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<DownloadCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class DownloadPatriciaTree : public BasicDownloadPatriciaTree {
	public:
		using BasicDownloadPatriciaTree::BasicDownloadPatriciaTree;
		using Serializer = DownloadCacheDescriptor::Serializer;
	};

	using DownloadSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<DownloadCacheTypes::PrimaryTypes, DownloadPatriciaTree>;

	struct DownloadBaseSetDeltaPointers : public DownloadSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct DownloadBaseSets : public DownloadSingleSetCacheTypesAdapter::BaseSets<DownloadBaseSetDeltaPointers> {
		using DownloadSingleSetCacheTypesAdapter::BaseSets<DownloadBaseSetDeltaPointers>::BaseSets;
	};
}}
