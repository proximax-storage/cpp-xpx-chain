/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractCacheSerializers.h"
#include "SuperContractCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicSuperContractPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<SuperContractCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class SuperContractPatriciaTree : public BasicSuperContractPatriciaTree {
	public:
		using BasicSuperContractPatriciaTree::BasicSuperContractPatriciaTree;
		using Serializer = SuperContractCacheDescriptor::Serializer;
	};

	using SuperContractSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<SuperContractCacheTypes::PrimaryTypes, SuperContractPatriciaTree>;

	struct SuperContractBaseSetDeltaPointers {
		SuperContractCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		std::shared_ptr<SuperContractPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct SuperContractBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit SuperContractBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		SuperContractCacheTypes::PrimaryTypes::BaseSetType Primary;
		CachePatriciaTree<SuperContractPatriciaTree> PatriciaTree;

	public:
		SuperContractBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), PatriciaTree.rebase() };
		}

		SuperContractBaseSetDeltaPointers rebaseDetached() const {
			return {
					Primary.rebaseDetached(),
					PatriciaTree.rebaseDetached()
			};
		}

		void commit() {
			Primary.commit();
			PatriciaTree.commit();
			flush();
		}
	};
}}
