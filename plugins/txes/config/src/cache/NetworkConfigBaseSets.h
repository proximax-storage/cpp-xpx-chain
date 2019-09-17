/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NetworkConfigCacheSerializers.h"
#include "NetworkConfigCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

	using BasicNetworkConfigPatriciaTree = tree::BasePatriciaTree<
		SerializerHashedKeyEncoder<NetworkConfigCacheDescriptor::Serializer>,
		PatriciaTreeRdbDataSource,
		utils::BaseValueHasher<Height>>;

	class NetworkConfigPatriciaTree : public BasicNetworkConfigPatriciaTree {
	public:
		using BasicNetworkConfigPatriciaTree::BasicNetworkConfigPatriciaTree;
		using Serializer = NetworkConfigCacheDescriptor::Serializer;
	};

	struct NetworkConfigBaseSetDeltaPointers {
		NetworkConfigCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
		NetworkConfigCacheTypes::HeightTypes::BaseSetDeltaPointerType pDeltaHeights;
		const NetworkConfigCacheTypes::HeightTypes::BaseSetType& PrimaryHeights;
		std::shared_ptr<NetworkConfigPatriciaTree::DeltaType> pPatriciaTree;
	};

	struct NetworkConfigBaseSets : public CacheDatabaseMixin {
	public:
		/// Indicates the set is not ordered.
		using IsOrderedSet = std::false_type;

	public:
		explicit NetworkConfigBaseSets(const CacheConfiguration& config)
				: CacheDatabaseMixin(config, { "default" })
				, Primary(GetContainerMode(config), database(), 0)
				, Heights(deltaset::ConditionalContainerMode::Memory, database(), -1)
				, PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
		{}

	public:
		NetworkConfigCacheTypes::PrimaryTypes::BaseSetType Primary;
		NetworkConfigCacheTypes::HeightTypes::BaseSetType Heights;
		CachePatriciaTree<NetworkConfigPatriciaTree> PatriciaTree;

	public:
		NetworkConfigBaseSetDeltaPointers rebase() {
			return { Primary.rebase(), Heights.rebase(), Heights, PatriciaTree.rebase() };
		}

		NetworkConfigBaseSetDeltaPointers rebaseDetached() const {
			return {
				Primary.rebaseDetached(),
				Heights.rebaseDetached(),
				Heights,
				PatriciaTree.rebaseDetached()
			};
		}

		void commit() {
			Primary.commit();
			Heights.commit(deltaset::PruningBoundary<Height>());
			PatriciaTree.commit();
			flush();
		}
	};
}}
