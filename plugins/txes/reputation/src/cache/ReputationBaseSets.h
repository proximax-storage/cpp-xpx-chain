/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "ReputationCacheSerializers.h"
#include "ReputationCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicReputationPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<ReputationCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class ReputationPatriciaTree : public BasicReputationPatriciaTree {
	public:
		using BasicReputationPatriciaTree::BasicReputationPatriciaTree;
		using Serializer = ReputationCacheDescriptor::Serializer;
	};

	using ReputationSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<ReputationCacheTypes::PrimaryTypes, ReputationPatriciaTree>;

	struct ReputationBaseSetDeltaPointers : public ReputationSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
	};

	struct ReputationBaseSets : public ReputationSingleSetCacheTypesAdapter::BaseSets<ReputationBaseSetDeltaPointers> {
		using ReputationSingleSetCacheTypesAdapter::BaseSets<ReputationBaseSetDeltaPointers>::BaseSets;
	};
}}
