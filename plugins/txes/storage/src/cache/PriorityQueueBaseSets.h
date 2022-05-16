/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "PriorityQueueCacheSerializers.h"
#include "PriorityQueueCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicPriorityQueuePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<PriorityQueueCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class PriorityQueuePatriciaTree : public BasicPriorityQueuePatriciaTree {
	public:
		using BasicPriorityQueuePatriciaTree::BasicPriorityQueuePatriciaTree;
		using Serializer = PriorityQueueCacheDescriptor::Serializer;
	};

	using PriorityQueueSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<PriorityQueueCacheTypes::PrimaryTypes, PriorityQueuePatriciaTree>;

	struct PriorityQueueBaseSetDeltaPointers {
		PriorityQueueCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		std::shared_ptr<PriorityQueuePatriciaTree::DeltaType> pPatriciaTree;
	};

	struct PriorityQueueBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit PriorityQueueBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default"})
				, Primary(GetContainerMode(config), database(), 0)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		PriorityQueueCacheTypes::PrimaryTypes::BaseSetType Primary;
		CachePatriciaTree<PriorityQueuePatriciaTree> PatriciaTree;

	public:
		PriorityQueueBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), PatriciaTree.rebase() };
		}

		PriorityQueueBaseSetDeltaPointers rebaseDetached() const {
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
