/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaExchangeCacheSerializers.h"
#include "SdaExchangeCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicSdaExchangePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<SdaExchangeEntryPatriciaTreeSerializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class SdaExchangePatriciaTree : public BasicSdaExchangePatriciaTree {
	public:
		using BasicSdaExchangePatriciaTree::BasicSdaExchangePatriciaTree;
		using Serializer = SdaExchangeEntryPatriciaTreeSerializer;
	};

	struct SdaExchangeBaseSetDeltaPointers {
		SdaExchangeCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		SdaExchangeCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType pHeightGrouping;
		std::shared_ptr<SdaExchangePatriciaTree::DeltaType> pPatriciaTree;
	};

	struct SdaExchangeBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit SdaExchangeBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default", "height_grouping" })
				, Primary(GetContainerMode(config), database(), 0)
				, HeightGrouping(GetContainerMode(config), database(), 1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 2)
		{}

	public:
		SdaExchangeCacheTypes::PrimaryTypes::BaseSetType Primary;
		SdaExchangeCacheTypes::HeightGroupingTypes::BaseSetType HeightGrouping;
		CachePatriciaTree<SdaExchangePatriciaTree> PatriciaTree;

	public:
		SdaExchangeBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), HeightGrouping.rebase(), PatriciaTree.rebase() };
		}

		SdaExchangeBaseSetDeltaPointers rebaseDetached() const {
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
