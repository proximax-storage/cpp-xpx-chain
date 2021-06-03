/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReplicatorCacheSerializers.h"
#include "ReplicatorCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicReplicatorPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<ReplicatorCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class ReplicatorPatriciaTree : public BasicReplicatorPatriciaTree {
	public:
		using BasicReplicatorPatriciaTree::BasicReplicatorPatriciaTree;
		using Serializer = ReplicatorCacheDescriptor::Serializer;
	};

	struct ReplicatorBaseSetDeltaPointers {
		ReplicatorCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		ReplicatorCacheTypes::KeyTypes::BaseSetDeltaPointerType pDeltaKeys;
		const ReplicatorCacheTypes::KeyTypes::BaseSetType& PrimaryKeys;
		std::shared_ptr<ReplicatorPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct ReplicatorBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit ReplicatorBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, Keys(deltaset::ConditionalContainerMode::Memory, database(), -1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		ReplicatorCacheTypes::PrimaryTypes::BaseSetType Primary;
		ReplicatorCacheTypes::KeyTypes::BaseSetType Keys;
		CachePatriciaTree<ReplicatorPatriciaTree> PatriciaTree;

	public:
		ReplicatorBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), Keys.rebase(), Keys, PatriciaTree.rebase() };
		}

		ReplicatorBaseSetDeltaPointers rebaseDetached() const {
			return {
				Primary.rebaseDetached(),
				Keys.rebaseDetached(),
				Keys,
				PatriciaTree.rebaseDetached()
			};
		}

		void commit() {
			Primary.commit();
			Keys.commit();
			PatriciaTree.commit();
			flush();
		}
	};
}}
