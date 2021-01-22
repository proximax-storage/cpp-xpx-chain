/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeCacheSerializers.h"
#include "CommitteeCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicCommitteePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<CommitteeCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class CommitteePatriciaTree : public BasicCommitteePatriciaTree {
	public:
		using BasicCommitteePatriciaTree::BasicCommitteePatriciaTree;
		using Serializer = CommitteeCacheDescriptor::Serializer;
	};

	struct CommitteeBaseSetDeltaPointers {
		CommitteeCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		CommitteeCacheTypes::KeyTypes::BaseSetDeltaPointerType pDeltaKeys;
		const CommitteeCacheTypes::KeyTypes::BaseSetType& PrimaryKeys;
		std::shared_ptr<CommitteePatriciaTree::DeltaType> pPatriciaTree;
	};

	struct CommitteeBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit CommitteeBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, Keys(deltaset::ConditionalContainerMode::Memory, database(), -1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		CommitteeCacheTypes::PrimaryTypes::BaseSetType Primary;
		CommitteeCacheTypes::KeyTypes::BaseSetType Keys;
		CachePatriciaTree<CommitteePatriciaTree> PatriciaTree;

	public:
		CommitteeBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), Keys.rebase(), Keys, PatriciaTree.rebase() };
		}

		CommitteeBaseSetDeltaPointers rebaseDetached() const {
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
