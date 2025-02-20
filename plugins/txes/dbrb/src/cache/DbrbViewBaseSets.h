/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbViewCacheSerializers.h"
#include "DbrbViewCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicDbrbViewPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<DbrbViewCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<dbrb::ProcessId>>;

	class DbrbViewPatriciaTree : public BasicDbrbViewPatriciaTree {
	public:
		using BasicDbrbViewPatriciaTree::BasicDbrbViewPatriciaTree;
		using Serializer = DbrbViewCacheDescriptor::Serializer;
	};

	struct DbrbViewBaseSetDeltaPointers {
		DbrbViewCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		DbrbViewCacheTypes::ProcessIdTypes::BaseSetDeltaPointerType pDeltaProcessIds;
		const DbrbViewCacheTypes::ProcessIdTypes::BaseSetType& PrimaryProcessIds;
		std::shared_ptr<DbrbViewPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct DbrbViewBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit DbrbViewBaseSets(const CacheConfiguration& config)
			: CacheDatabaseMixin(config, { "default" })
			, Primary(GetContainerMode(config), database(), 0)
			, ProcessIds(deltaset::ConditionalContainerMode::Memory, database(), -1)
			, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		DbrbViewCacheTypes::PrimaryTypes::BaseSetType Primary;
		DbrbViewCacheTypes::ProcessIdTypes::BaseSetType ProcessIds;
		CachePatriciaTree<DbrbViewPatriciaTree> PatriciaTree;

	public:
		DbrbViewBaseSetDeltaPointers rebase() {
			return {
				Primary.rebase(),
				ProcessIds.rebase(),
				ProcessIds,
				PatriciaTree.rebase()
			};
		}

		DbrbViewBaseSetDeltaPointers rebaseDetached() const {
			return {
				Primary.rebaseDetached(),
				ProcessIds.rebaseDetached(),
				ProcessIds,
				PatriciaTree.rebaseDetached()
			};
		}

		void commit() {
			Primary.commit();
			ProcessIds.commit();
			PatriciaTree.commit();
			flush();
		}
	};
}}
