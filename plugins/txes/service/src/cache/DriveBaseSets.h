/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
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
		DriveCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType pDriveEndHeightGrouping;
		DriveCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType pVerificationEndHeightGrouping;
		std::shared_ptr<DrivePatriciaTree::DeltaType> pPatriciaTree;
	};

	struct DriveBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit DriveBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default", "drive_end_height_grouping", "verification_end_height_grouping" })
				, Primary(GetContainerMode(config), database(), 0)
				, DriveEndHeightGrouping(GetContainerMode(config), database(), 1)
				, VerificationEndHeightGrouping(GetContainerMode(config), database(), 2)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 3)
		{}

	public:
		DriveCacheTypes::PrimaryTypes::BaseSetType Primary;
		DriveCacheTypes::HeightGroupingTypes::BaseSetType DriveEndHeightGrouping;
		DriveCacheTypes::HeightGroupingTypes::BaseSetType VerificationEndHeightGrouping;
		CachePatriciaTree<DrivePatriciaTree> PatriciaTree;

	public:
		DriveBaseSetDeltaPointers rebase() {
			return {
				Primary.rebase(),
				DriveEndHeightGrouping.rebase(),
				VerificationEndHeightGrouping.rebase(),
				PatriciaTree.rebase()
			};
		}

		DriveBaseSetDeltaPointers rebaseDetached() const {
			return {
					Primary.rebaseDetached(),
					DriveEndHeightGrouping.rebaseDetached(),
					VerificationEndHeightGrouping.rebaseDetached(),
					PatriciaTree.rebaseDetached()
			};
		}

		void commit() {
			Primary.commit();
			DriveEndHeightGrouping.commit();
			VerificationEndHeightGrouping.commit();
			PatriciaTree.commit();
			flush();
		}
	};
}}
