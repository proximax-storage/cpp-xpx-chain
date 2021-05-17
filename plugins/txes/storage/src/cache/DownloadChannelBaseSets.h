/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadChannelCacheSerializers.h"
#include "DownloadChannelCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicDownloadChannelPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<DownloadChannelCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<Key>>;

	class DownloadChannelPatriciaTree : public BasicDownloadChannelPatriciaTree {
	public:
		using BasicDownloadChannelPatriciaTree::BasicDownloadChannelPatriciaTree;
		using Serializer = DownloadChannelCacheDescriptor::Serializer;
	};

	using DownloadChannelSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<DownloadChannelCacheTypes::PrimaryTypes, DownloadChannelPatriciaTree>;

	struct DownloadChannelBaseSetDeltaPointers {
		DownloadChannelCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		std::shared_ptr<DownloadChannelPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct DownloadChannelBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit DownloadChannelBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		DownloadChannelCacheTypes::PrimaryTypes::BaseSetType Primary;
		CachePatriciaTree<DownloadChannelPatriciaTree> PatriciaTree;

	public:
		DownloadChannelBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), PatriciaTree.rebase() };
		}

		DownloadChannelBaseSetDeltaPointers rebaseDetached() const {
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
