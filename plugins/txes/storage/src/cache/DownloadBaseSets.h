/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadCacheSerializers.h"
#include "DownloadCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicDownloadPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<DownloadCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class DownloadPatriciaTree : public BasicDownloadPatriciaTree {
	public:
		using BasicDownloadPatriciaTree::BasicDownloadPatriciaTree;
		using Serializer = DownloadCacheDescriptor::Serializer;
	};

	using DownloadSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<DownloadCacheTypes::PrimaryTypes, DownloadPatriciaTree>;

	struct DownloadBaseSetDeltaPointers {
		DownloadCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		std::shared_ptr<DownloadPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct DownloadBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit DownloadBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		DownloadCacheTypes::PrimaryTypes::BaseSetType Primary;
		CachePatriciaTree<DownloadPatriciaTree> PatriciaTree;

	public:
		DownloadBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), PatriciaTree.rebase() };
		}

		DownloadBaseSetDeltaPointers rebaseDetached() const {
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
