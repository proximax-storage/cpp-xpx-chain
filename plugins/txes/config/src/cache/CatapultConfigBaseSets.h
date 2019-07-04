/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultConfigCacheSerializers.h"
#include "CatapultConfigCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicCatapultConfigPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<CatapultConfigCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<Height>>;

	class CatapultConfigPatriciaTree : public BasicCatapultConfigPatriciaTree {
	public:
		using BasicCatapultConfigPatriciaTree::BasicCatapultConfigPatriciaTree;
		using Serializer = CatapultConfigCacheDescriptor::Serializer;
	};

	struct CatapultConfigBaseSetDeltaPointers {
		CatapultConfigCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		CatapultConfigCacheTypes::HeightTypes::BaseSetDeltaPointerType pHeights;
		std::shared_ptr<CatapultConfigPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct CatapultConfigBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit CatapultConfigBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default", "heights" })
				, Primary(GetContainerMode(config), database(), 0)
				, Heights(GetContainerMode(config), database(), 1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 2)
		{}

	public:
		CatapultConfigCacheTypes::PrimaryTypes::BaseSetType Primary;
		CatapultConfigCacheTypes::HeightTypes::BaseSetType Heights;
		CachePatriciaTree<CatapultConfigPatriciaTree> PatriciaTree;

	public:
		CatapultConfigBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), Heights.rebase(), PatriciaTree.rebase() };
		}

		CatapultConfigBaseSetDeltaPointers rebaseDetached() const {
			return {
				Primary.rebaseDetached(),
				Heights.rebaseDetached(),
				PatriciaTree.rebaseDetached()
			};
		}

		void commit() {
			Primary.commit();
			Heights.commit(deltaset::PruningBoundary<Height>());
			PatriciaTree.commit();
			flush();
		}
	};
}}
