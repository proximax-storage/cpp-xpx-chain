/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveContractCacheSerializers.h"
#include "DriveContractCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicDriveContractPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<DriveContractCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class DriveContractPatriciaTree : public BasicDriveContractPatriciaTree {
	public:
		using BasicDriveContractPatriciaTree::BasicDriveContractPatriciaTree;
		using Serializer = DriveContractCacheDescriptor::Serializer;
	};

	using DriveContractSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<DriveContractCacheTypes::PrimaryTypes, DriveContractPatriciaTree>;

	struct DriveContractBaseSetDeltaPointers {
		DriveContractCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		std::shared_ptr<DriveContractPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct DriveContractBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit DriveContractBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		DriveContractCacheTypes::PrimaryTypes::BaseSetType Primary;
		CachePatriciaTree<DriveContractPatriciaTree> PatriciaTree;

	public:
		DriveContractBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), PatriciaTree.rebase() };
		}

		DriveContractBaseSetDeltaPointers rebaseDetached() const {
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
