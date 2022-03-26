/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LPCacheSerializers.h"
#include "LPCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicLPPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<LPCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<MosaicId>>;

	class LPPatriciaTree : public BasicLPPatriciaTree {
	public:
		using BasicLPPatriciaTree::BasicLPPatriciaTree;
		using Serializer = LPCacheDescriptor::Serializer;
	};

	struct LPBaseSetDeltaPointers {
		LPCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		LPCacheTypes::KeyTypes::BaseSetDeltaPointerType pDeltaKeys;
		const LPCacheTypes::KeyTypes::BaseSetType& PrimaryKeys;
		std::shared_ptr<LPPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct LPBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit LPBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, Keys(deltaset::ConditionalContainerMode::Memory, database(), -1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		LPCacheTypes::PrimaryTypes::BaseSetType Primary;
		LPCacheTypes::KeyTypes::BaseSetType Keys;
		CachePatriciaTree<LPPatriciaTree> PatriciaTree;

	public:
		LPBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), Keys.rebase(), Keys, PatriciaTree.rebase() };
		}

		LPBaseSetDeltaPointers rebaseDetached() const {
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
