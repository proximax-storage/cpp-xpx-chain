/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/ExchangeEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"
#include "catapult/utils/ShortHash.h"

namespace catapult {
	namespace cache {
		class BasicExchangeCacheDelta;
		class BasicExchangeCacheView;
		struct ExchangeBaseSetDeltaPointers;
		struct ExchangeBaseSets;
		class ExchangeCache;
		class ExchangeCacheDelta;
		class ExchangeCacheView;
		struct ExchangeEntryPrimarySerializer;
		struct ExchangeHeightGroupingSerializer;
		class ExchangePatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a exchange cache.
	struct ExchangeCacheDescriptor {
	public:
		static constexpr auto Name = "ExchangeCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::ExchangeEntry;

		// cache types
		using CacheType = ExchangeCache;
		using CacheDeltaType = ExchangeCacheDelta;
		using CacheViewType = ExchangeCacheView;

		using Serializer = ExchangeEntryPrimarySerializer;
		using PatriciaTree = ExchangePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.owner();
		}
	};

	/// Exchange cache types.
	struct ExchangeCacheTypes {
	public:
		struct HeightGroupingTypesDescriptor {
		public:
			using KeyType = Height;
			using ValueType = utils::IdentifierGroup<Key, Height, utils::ArrayHasher<Key>>;
			using Serializer = ExchangeHeightGroupingSerializer;

		public:
			static auto GetKeyFromValue(const ValueType& heightExchange) {
				return heightExchange.key();
			}
		};

		using PrimaryTypes = MutableUnorderedMapAdapter<ExchangeCacheDescriptor, utils::ArrayHasher<Key>>;
		using HeightGroupingTypes = MutableUnorderedMapAdapter<HeightGroupingTypesDescriptor, utils::BaseValueHasher<Height>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicExchangeCacheView, BasicExchangeCacheDelta, const Key&, state::ExchangeEntry>;

		using BaseSetDeltaPointers = ExchangeBaseSetDeltaPointers;
		using BaseSets = ExchangeBaseSets;
	};
}}
