/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveCacheSerializers.h"
#include "DriveCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicDrivePatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<DriveCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class DrivePatriciaTree : public BasicDrivePatriciaTree {
	public:
		using BasicDrivePatriciaTree::BasicDrivePatriciaTree;
		using Serializer = DriveCacheDescriptor::Serializer;
	};

	using DriveSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<DriveCacheTypes::PrimaryTypes, DrivePatriciaTree>;

	struct DriveBaseSetDeltaPointers {
		DriveCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		DriveCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType pBillingGrouping;
		DriveCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType pRemoveGrouping;
		std::shared_ptr<DrivePatriciaTree::DeltaType> pPatriciaTree;
	};

	struct DriveBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit DriveBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default", "billing_height_grouping", "remove_height_grouping" })
				, Primary(GetContainerMode(config), database(), 0)
				, BillingGrouping(GetContainerMode(config), database(), 1)
				, RemoveGrouping(GetContainerMode(config), database(), 2)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 3)
		{}

	public:
		DriveCacheTypes::PrimaryTypes::BaseSetType Primary;
		DriveCacheTypes::HeightGroupingTypes::BaseSetType BillingGrouping;
		DriveCacheTypes::HeightGroupingTypes::BaseSetType RemoveGrouping;
		CachePatriciaTree<DrivePatriciaTree> PatriciaTree;

	public:
		DriveBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), BillingGrouping.rebase(), RemoveGrouping.rebase(), PatriciaTree.rebase() };
		}

		DriveBaseSetDeltaPointers rebaseDetached() const {
			return {
					Primary.rebaseDetached(),
					BillingGrouping.rebaseDetached(),
					RemoveGrouping.rebaseDetached(),
					PatriciaTree.rebaseDetached()
			};
		}

		void commit() {
			Primary.commit();
			BillingGrouping.commit();
			RemoveGrouping.commit();
			PatriciaTree.commit();
			flush();
		}
	};
}}
