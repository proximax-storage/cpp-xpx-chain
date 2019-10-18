/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BuyOfferCacheSerializers.h"
#include "BuyOfferCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicBuyOfferPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<BuyOfferCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<utils::ShortHash>>;

	class BuyOfferPatriciaTree : public BasicBuyOfferPatriciaTree {
	public:
		using BasicBuyOfferPatriciaTree::BasicBuyOfferPatriciaTree;
		using Serializer = BuyOfferCacheDescriptor::Serializer;
	};

	using BuyOfferSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<BuyOfferCacheTypes::PrimaryTypes, BuyOfferPatriciaTree>;

	struct BuyOfferBaseSetDeltaPointers : public BuyOfferSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct BuyOfferBaseSets : public BuyOfferSingleSetCacheTypesAdapter::BaseSets<BuyOfferBaseSetDeltaPointers> {
		using BuyOfferSingleSetCacheTypesAdapter::BaseSets<BuyOfferBaseSetDeltaPointers>::BaseSets;
	};
}}
