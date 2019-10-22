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

	struct OfferBaseSetDeltaPointers {
		OfferCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		OfferCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType pHeightGrouping;
		std::shared_ptr<OfferPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct OfferBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit OfferBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default", "key_lookup" })
				, Primary(GetContainerMode(config), database(), 0)
				, HeightGrouping(GetContainerMode(config), database(), 1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 2)
		{}

	public:
		OfferCacheTypes::PrimaryTypes::BaseSetType Primary;
		OfferCacheTypes::HeightGroupingTypes::BaseSetType HeightGrouping;
		CachePatriciaTree<OfferPatriciaTree> PatriciaTree;

	public:
		OfferBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), HeightGrouping.rebase(), PatriciaTree.rebase() };
		}

		OfferBaseSetDeltaPointers rebaseDetached() const {
			return { Primary.rebaseDetached(), HeightGrouping.rebaseDetached(), PatriciaTree.rebaseDetached() };
		}

		void commit() {
			Primary.commit();
			HeightGrouping.commit();
			PatriciaTree.commit();
			flush();
		}
	};
}}
