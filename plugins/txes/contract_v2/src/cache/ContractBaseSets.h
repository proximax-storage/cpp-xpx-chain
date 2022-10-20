/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ContractCacheSerializers.h"
#include "ContractCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicContractPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<ContractCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class ContractPatriciaTree : public BasicContractPatriciaTree {
	public:
		using BasicContractPatriciaTree::BasicContractPatriciaTree;
		using Serializer = ContractCacheDescriptor::Serializer;
	};

	using ContractSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<ContractCacheTypes::PrimaryTypes, ContractPatriciaTree>;

	struct ContractBaseSetDeltaPointers {
		ContractCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		std::shared_ptr<ContractPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct ContractBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit ContractBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		ContractCacheTypes::PrimaryTypes::BaseSetType Primary;
		CachePatriciaTree<ContractPatriciaTree> PatriciaTree;

	public:
		ContractBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), PatriciaTree.rebase() };
		}

		ContractBaseSetDeltaPointers rebaseDetached() const {
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
