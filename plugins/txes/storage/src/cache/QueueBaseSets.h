/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "QueueCacheSerializers.h"
#include "QueueCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicQueuePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<QueueCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class QueuePatriciaTree : public BasicQueuePatriciaTree {
	public:
		using BasicQueuePatriciaTree::BasicQueuePatriciaTree;
		using Serializer = QueueCacheDescriptor::Serializer;
	};

	struct QueueBaseSetDeltaPointers {
		QueueCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		QueueCacheTypes::KeyTypes::BaseSetDeltaPointerType pDeltaKeys;
		const QueueCacheTypes::KeyTypes::BaseSetType& PrimaryKeys;
		std::shared_ptr<QueuePatriciaTree::DeltaType> pPatriciaTree;
	};

	struct QueueBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit QueueBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, Keys(deltaset::ConditionalContainerMode::Memory, database(), -1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		QueueCacheTypes::PrimaryTypes::BaseSetType Primary;
		QueueCacheTypes::KeyTypes::BaseSetType Keys;
		CachePatriciaTree<QueuePatriciaTree> PatriciaTree;

	public:
		QueueBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), Keys.rebase(), Keys, PatriciaTree.rebase() };
		}

		QueueBaseSetDeltaPointers rebaseDetached() const {
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
