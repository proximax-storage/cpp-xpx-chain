/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeCacheSerializers.h"
#include "ExchangeCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicExchangePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<ExchangeCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class ExchangePatriciaTree : public BasicExchangePatriciaTree {
	public:
		using BasicExchangePatriciaTree::BasicExchangePatriciaTree;
		using Serializer = ExchangeCacheDescriptor::Serializer;
	};

	struct ExchangeBaseSetDeltaPointers {
		ExchangeCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		ExchangeCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType pHeightGrouping;
		std::shared_ptr<ExchangePatriciaTree::DeltaType> pPatriciaTree;
	};

	struct ExchangeBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit ExchangeBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default", "height_grouping" })
				, Primary(GetContainerMode(config), database(), 0)
				, HeightGrouping(GetContainerMode(config), database(), 1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 2)
		{}

	public:
		ExchangeCacheTypes::PrimaryTypes::BaseSetType Primary;
		ExchangeCacheTypes::HeightGroupingTypes::BaseSetType HeightGrouping;
		CachePatriciaTree<ExchangePatriciaTree> PatriciaTree;

	public:
		ExchangeBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), HeightGrouping.rebase(), PatriciaTree.rebase() };
		}

		ExchangeBaseSetDeltaPointers rebaseDetached() const {
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
