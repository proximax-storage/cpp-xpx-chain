/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
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

	struct ContractBaseSetDeltaPointers : public ContractSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct ContractBaseSets : public ContractSingleSetCacheTypesAdapter::BaseSets<ContractBaseSetDeltaPointers> {
		using ContractSingleSetCacheTypesAdapter::BaseSets<ContractBaseSetDeltaPointers>::BaseSets;
	};
}}
