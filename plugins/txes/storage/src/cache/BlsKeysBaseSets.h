/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BlsKeysCacheSerializers.h"
#include "BlsKeysCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicBlsKeysPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<BlsKeysCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::ArrayHasher<BLSPublicKey>>;

	class BlsKeysPatriciaTree : public BasicBlsKeysPatriciaTree {
	public:
		using BasicBlsKeysPatriciaTree::BasicBlsKeysPatriciaTree;
		using Serializer = BlsKeysCacheDescriptor::Serializer;
	};

	using BlsKeysSingleSetCacheTypesAdapter =
		SingleSetAndPatriciaTreeCacheTypesAdapter<BlsKeysCacheTypes::PrimaryTypes, BlsKeysPatriciaTree>;

	struct BlsKeysBaseSetDeltaPointers {
		BlsKeysCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		std::shared_ptr<BlsKeysPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct BlsKeysBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit BlsKeysBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default"})
				, Primary(GetContainerMode(config), database(), 0)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		BlsKeysCacheTypes::PrimaryTypes::BaseSetType Primary;
		CachePatriciaTree<BlsKeysPatriciaTree> PatriciaTree;

	public:
		BlsKeysBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), PatriciaTree.rebase() };
		}

		BlsKeysBaseSetDeltaPointers rebaseDetached() const {
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
