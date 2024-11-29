/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataV1CacheSerializers.h"
#include "MetadataV1CacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicMetadataV1PatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<MetadataV1CacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Hash256>>;

	class MetadataV1PatriciaTree : public BasicMetadataV1PatriciaTree {
	public:
		using BasicMetadataV1PatriciaTree::BasicMetadataV1PatriciaTree;
		using Serializer = MetadataV1CacheDescriptor::Serializer;
	};

	struct MetadataV1BaseSetDeltaPointers {
		MetadataV1CacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		MetadataV1CacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType pHeightGrouping;
		std::shared_ptr<MetadataV1PatriciaTree::DeltaType> pPatriciaTree;
	};

	struct MetadataV1BaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit MetadataV1BaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default", "height_grouping" })
				, Primary(GetContainerMode(config), database(), 0)
				, HeightGrouping(GetContainerMode(config), database(), 1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 2)
		{}

	public:
		MetadataV1CacheTypes::PrimaryTypes::BaseSetType Primary;
		MetadataV1CacheTypes::HeightGroupingTypes::BaseSetType HeightGrouping;
		CachePatriciaTree<MetadataV1PatriciaTree> PatriciaTree;

	public:
		MetadataV1BaseSetDeltaPointers rebase() {
			return { Primary.rebase(), HeightGrouping.rebase(), PatriciaTree.rebase() };
		}

		MetadataV1BaseSetDeltaPointers rebaseDetached() const {
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
