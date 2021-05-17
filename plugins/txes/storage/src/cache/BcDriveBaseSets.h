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

	using BcDriveSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<BcDriveCacheTypes::PrimaryTypes, BcDrivePatriciaTree>;

	struct BcDriveBaseSetDeltaPointers {
		BcDriveCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		std::shared_ptr<BcDrivePatriciaTree::DeltaType> pPatriciaTree;
	};

	struct BcDriveBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit BcDriveBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default"})
				, Primary(GetContainerMode(config), database(), 0)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		BcDriveCacheTypes::PrimaryTypes::BaseSetType Primary;
		CachePatriciaTree<BcDrivePatriciaTree> PatriciaTree;

	public:
		BcDriveBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), PatriciaTree.rebase() };
		}

		BcDriveBaseSetDeltaPointers rebaseDetached() const {
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
