/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BcDriveCacheSerializers.h"
#include "BcDriveCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicBcDrivePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<BcDriveCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class BcDrivePatriciaTree : public BasicBcDrivePatriciaTree {
	public:
		using BasicBcDrivePatriciaTree::BasicBcDrivePatriciaTree;
		using Serializer = BcDriveCacheDescriptor::Serializer;
	};

	struct BcDriveBaseSetDeltaPointers {
		BcDriveCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		BcDriveCacheTypes::KeyTypes::BaseSetDeltaPointerType pDeltaKeys;
		const BcDriveCacheTypes::KeyTypes::BaseSetType& PrimaryKeys;
		std::shared_ptr<BcDrivePatriciaTree::DeltaType> pPatriciaTree;
	};

	struct BcDriveBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::true_type;

	public:
		explicit BcDriveBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, Keys(deltaset::ConditionalContainerMode::Memory, database(), -1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		BcDriveCacheTypes::PrimaryTypes::BaseSetType Primary;
		BcDriveCacheTypes::KeyTypes::BaseSetType Keys;
		CachePatriciaTree<BcDrivePatriciaTree> PatriciaTree;

	public:
		BcDriveBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), Keys.rebase(), Keys, PatriciaTree.rebase() };
		}

		BcDriveBaseSetDeltaPointers rebaseDetached() const {
			return {
				Primary.rebaseDetached(),
				Keys.rebaseDetached(),
				Keys,
				PatriciaTree.rebaseDetached()
			};
		}

		void commit(const deltaset::PruningBoundary<Key>& boundary) {
			Primary.commit();
			Keys.commit(boundary);
			PatriciaTree.commit();
			flush();
		}
	};
}}
