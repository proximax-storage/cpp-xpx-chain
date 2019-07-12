/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/ContractEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicContractCacheDelta;
		class BasicContractCacheView;
		struct ContractBaseSetDeltaPointers;
		struct ContractBaseSets;
		class ContractCache;
		class ContractCacheDelta;
		class ContractCacheView;
		struct ContractEntryPrimarySerializer;
		class ContractPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a contract cache.
	struct ContractCacheDescriptor {
	public:
		static constexpr auto Name = "ContractCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::ContractEntry;

		// cache types
		using CacheType = ContractCache;
		using CacheDeltaType = ContractCacheDelta;
		using CacheViewType = ContractCacheView;

		using Serializer = ContractEntryPrimarySerializer;
		using PatriciaTree = ContractPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// Contract cache types.
	struct ContractCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<ContractCacheDescriptor, utils::ArrayHasher<Key>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicContractCacheView, BasicContractCacheDelta, const Key&, state::ContractEntry>;

		using BaseSetDeltaPointers = ContractBaseSetDeltaPointers;
		using BaseSets = ContractBaseSets;
	};
}}
