/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SellOfferCacheSerializers.h"
#include "SellOfferCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicSellOfferPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<SellOfferCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<utils::ShortHash>>;

	class SellOfferPatriciaTree : public BasicSellOfferPatriciaTree {
	public:
		using BasicSellOfferPatriciaTree::BasicSellOfferPatriciaTree;
		using Serializer = SellOfferCacheDescriptor::Serializer;
	};

	using SellOfferSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<SellOfferCacheTypes::PrimaryTypes, SellOfferPatriciaTree>;

	struct SellOfferBaseSetDeltaPointers : public SellOfferSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct SellOfferBaseSets : public SellOfferSingleSetCacheTypesAdapter::BaseSets<SellOfferBaseSetDeltaPointers> {
		using SellOfferSingleSetCacheTypesAdapter::BaseSets<SellOfferBaseSetDeltaPointers>::BaseSets;
	};
}}
