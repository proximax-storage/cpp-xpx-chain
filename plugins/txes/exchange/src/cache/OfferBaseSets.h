/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferCacheSerializers.h"
#include "OfferCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicOfferPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<OfferCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<utils::ShortHash>>;

	class OfferPatriciaTree : public BasicOfferPatriciaTree {
	public:
		using BasicOfferPatriciaTree::BasicOfferPatriciaTree;
		using Serializer = OfferCacheDescriptor::Serializer;
	};

	using OfferSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<OfferCacheTypes::PrimaryTypes, OfferPatriciaTree>;

	struct OfferBaseSetDeltaPointers : public OfferSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
		OfferCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType pHeightGrouping;
	};

	struct OfferBaseSets : public OfferSingleSetCacheTypesAdapter::BaseSets<OfferBaseSetDeltaPointers> {
		using OfferSingleSetCacheTypesAdapter::BaseSets<OfferBaseSetDeltaPointers>::BaseSets;
	};
}}
