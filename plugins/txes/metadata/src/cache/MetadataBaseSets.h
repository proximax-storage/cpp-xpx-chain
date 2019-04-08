/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataCacheSerializers.h"
#include "MetadataCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicMetadataPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<MetadataCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Hash256>>;

	class MetadataPatriciaTree : public BasicMetadataPatriciaTree {
	public:
		using BasicMetadataPatriciaTree::BasicMetadataPatriciaTree;
		using Serializer = MetadataCacheDescriptor::Serializer;
	};

	struct MetadataBaseSetDeltaPointers {
		MetadataCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		MetadataCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType pHeightGrouping;
		std::shared_ptr<MetadataPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct MetadataBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit MetadataBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default", "height_grouping" })
				, Primary(GetContainerMode(config), database(), 0)
				, HeightGrouping(GetContainerMode(config), database(), 1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 2)
		{}

	public:
		MetadataCacheTypes::PrimaryTypes::BaseSetType Primary;
		MetadataCacheTypes::HeightGroupingTypes::BaseSetType HeightGrouping;
		CachePatriciaTree<MetadataPatriciaTree> PatriciaTree;

	public:
		MetadataBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), HeightGrouping.rebase(), PatriciaTree.rebase() };
		}

		MetadataBaseSetDeltaPointers rebaseDetached() const {
			return {
				Primary.rebaseDetached(),
				HeightGrouping.rebaseDetached(),
				PatriciaTree.rebaseDetached()
			};
		}

		void commit() {
			Primary.commit();
			HeightGrouping.commit();
			PatriciaTree.commit();
			flush();
		}
	};
}}
