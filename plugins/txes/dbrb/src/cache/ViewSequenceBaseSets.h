/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ViewSequenceCacheSerializers.h"
#include "ViewSequenceCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicViewSequencePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<ViewSequenceCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Hash256>>;

	class ViewSequencePatriciaTree : public BasicViewSequencePatriciaTree {
	public:
		using BasicViewSequencePatriciaTree::BasicViewSequencePatriciaTree;
		using Serializer = ViewSequenceCacheDescriptor::Serializer;
	};

	using ViewSequenceSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<ViewSequenceCacheTypes::PrimaryTypes, ViewSequencePatriciaTree>;

	struct ViewSequenceBaseSetDeltaPointers {
		ViewSequenceCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		ViewSequenceCacheTypes::MessageHashTypes::BaseSetDeltaPointerType pMessageHash;
		std::shared_ptr<ViewSequencePatriciaTree::DeltaType> pPatriciaTree;
	};

	struct ViewSequenceBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit ViewSequenceBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default", "message_hash" })
				, Primary(GetContainerMode(config), database(), 0)
				, MessageHash(GetContainerMode(config), database(), 1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 2)
		{}

	public:
		ViewSequenceCacheTypes::PrimaryTypes::BaseSetType Primary;
		ViewSequenceCacheTypes::MessageHashTypes::BaseSetType MessageHash;
		CachePatriciaTree<ViewSequencePatriciaTree> PatriciaTree;

	public:
		ViewSequenceBaseSetDeltaPointers rebase() {
			return {
					Primary.rebase(),
					MessageHash.rebase(),
					PatriciaTree.rebase()
			};
		}

		ViewSequenceBaseSetDeltaPointers rebaseDetached() const {
			return {
					Primary.rebaseDetached(),
					MessageHash.rebaseDetached(),
					PatriciaTree.rebaseDetached()
			};
		}

		void commit() {
			Primary.commit();
			MessageHash.commit();
			PatriciaTree.commit();
			flush();
		}
	};
}}
