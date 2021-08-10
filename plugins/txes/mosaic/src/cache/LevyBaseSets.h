/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "LevyCacheSerializers.h"
#include "LevyCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {
		
	using BasicLevyPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<LevyEntryPatriciaTreeSerializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;
		
	class LevyPatriciaTree : public BasicLevyPatriciaTree {
	public:
		using BasicLevyPatriciaTree::BasicLevyPatriciaTree;
		using Serializer = LevyEntryPatriciaTreeSerializer;
	};
		
	using LevySingleSetCacheTypesAdapter =
	SingleSetAndPatriciaTreeCacheTypesAdapter<LevyCacheTypes::PrimaryTypes, LevyPatriciaTree>;
	
	struct LevyBaseSetDeltaPointers {
		LevyCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		LevyCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType pHistoryAtHeight;
		std::shared_ptr<LevyPatriciaTree::DeltaType> pPatriciaTree;
	};
	
	struct LevyBaseSets : public CacheDatabaseMixin {

	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;
	
	public:
		explicit LevyBaseSets(const CacheConfiguration& config)
			: CacheDatabaseMixin(config, { "default", "history_height_grouping" })
			, Primary(GetContainerMode(config), database(), 0)
			, HistoryHeightGrouping(GetContainerMode(config), database(), 1)
			, PatriciaTree(hasPatriciaTreeSupport(), database(), 2)
		{}
	
	public:
		LevyCacheTypes::PrimaryTypes::BaseSetType Primary;
		LevyCacheTypes::HeightGroupingTypes::BaseSetType HistoryHeightGrouping;
		CachePatriciaTree<LevyPatriciaTree> PatriciaTree;
	
	public:
		LevyBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), HistoryHeightGrouping.rebase(), PatriciaTree.rebase() };
		}
		
		LevyBaseSetDeltaPointers rebaseDetached() const {
			return { Primary.rebaseDetached(), HistoryHeightGrouping.rebaseDetached(), PatriciaTree.rebaseDetached() };
		}
		
		void commit() {
			Primary.commit();
			HistoryHeightGrouping.commit();
			PatriciaTree.commit();
			flush();
		}
	};
}}

