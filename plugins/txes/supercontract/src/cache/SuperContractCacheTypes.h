/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/SuperContractEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicSuperContractCacheDelta;
		class BasicSuperContractCacheView;
		struct SuperContractBaseSetDeltaPointers;
		struct SuperContractBaseSets;
		class SuperContractCache;
		class SuperContractCacheDelta;
		class SuperContractCacheView;
		struct SuperContractEntryPrimarySerializer;
		class SuperContractPatriciaTree;
		struct SuperContractHeightGroupingSerializer;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a super contract cache.
	struct SuperContractCacheDescriptor {
	public:
		static constexpr auto Name = "SuperContractCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::SuperContractEntry;

		// cache types
		using CacheType = SuperContractCache;
		using CacheDeltaType = SuperContractCacheDelta;
		using CacheViewType = SuperContractCacheView;

		using Serializer = SuperContractEntryPrimarySerializer;
		using PatriciaTree = SuperContractPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// SuperContract cache types.
	struct SuperContractCacheTypes {
		struct HeightGroupingTypesDescriptor {
		public:
			using KeyType = Height;
			using ValueType = utils::IdentifierGroup<Key, Height, utils::ArrayHasher<Key>>;
			using Serializer = SuperContractHeightGroupingSerializer;

		public:
			static auto GetKeyFromValue(const ValueType& identifierGroup) {
				return identifierGroup.key();
			}
		};

		using PrimaryTypes = MutableUnorderedMapAdapter<SuperContractCacheDescriptor, utils::ArrayHasher<Key>>;
		using HeightGroupingTypes = MutableUnorderedMapAdapter<HeightGroupingTypesDescriptor, utils::BaseValueHasher<Height>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicSuperContractCacheView, BasicSuperContractCacheDelta, const Key&, state::SuperContractEntry>;

		using BaseSetDeltaPointers = SuperContractBaseSetDeltaPointers;
		using BaseSets = SuperContractBaseSets;
	};
}}
