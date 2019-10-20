/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferDeadlineCacheSerializers.h"
#include "OfferDeadlineCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicOfferDeadlinePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<OfferDeadlineCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<Height>>;

	class OfferDeadlinePatriciaTree : public BasicOfferDeadlinePatriciaTree {
	public:
		using BasicOfferDeadlinePatriciaTree::BasicOfferDeadlinePatriciaTree;
		using Serializer = OfferDeadlineCacheDescriptor::Serializer;
	};

	using OfferDeadlineSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<OfferDeadlineCacheTypes::PrimaryTypes, OfferDeadlinePatriciaTree>;

	struct OfferDeadlineBaseSetDeltaPointers : public OfferDeadlineSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct OfferDeadlineBaseSets : public OfferDeadlineSingleSetCacheTypesAdapter::BaseSets<OfferDeadlineBaseSetDeltaPointers> {
		using OfferDeadlineSingleSetCacheTypesAdapter::BaseSets<OfferDeadlineBaseSetDeltaPointers>::BaseSets;
	};
}}
