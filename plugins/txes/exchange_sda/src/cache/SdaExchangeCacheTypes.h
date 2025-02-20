/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/SdaExchangeEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicSdaExchangeCacheDelta;
		class BasicSdaExchangeCacheView;
		struct SdaExchangeBaseSetDeltaPointers;
		struct SdaExchangeBaseSets;
		class SdaExchangeCache;
		class SdaExchangeCacheDelta;
		class SdaExchangeCacheView;
		struct SdaExchangeEntryPrimarySerializer;
		struct SdaExchangeHeightGroupingSerializer;
		class SdaExchangePatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a SDA-SDA exchange cache.
	struct SdaExchangeCacheDescriptor {
	public:
		static constexpr auto Name = "SdaExchangeCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::SdaExchangeEntry;

		// cache types
		using CacheType = SdaExchangeCache;
		using CacheDeltaType = SdaExchangeCacheDelta;
		using CacheViewType = SdaExchangeCacheView;

		using Serializer = SdaExchangeEntryPrimarySerializer;
		using PatriciaTree = SdaExchangePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.owner();
		}
	};

	/// SDA-SDA Exchange cache types.
	struct SdaExchangeCacheTypes {
	public:
		struct HeightGroupingTypesDescriptor {
		public:
			using KeyType = Height;
			using ValueType = utils::OrderedIdentifierGroup<Key, Height>;
			using Serializer = SdaExchangeHeightGroupingSerializer;

		public:
			static auto GetKeyFromValue(const ValueType& heightSdaExchange) {
				return heightSdaExchange.key();
			}
		};

		using PrimaryTypes = MutableUnorderedMapAdapter<SdaExchangeCacheDescriptor, utils::ArrayHasher<Key>>;
		using HeightGroupingTypes = MutableUnorderedMapAdapter<HeightGroupingTypesDescriptor, utils::BaseValueHasher<Height>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicSdaExchangeCacheView, BasicSdaExchangeCacheDelta, const Key&, state::SdaExchangeEntry>;

		using BaseSetDeltaPointers = SdaExchangeBaseSetDeltaPointers;
		using BaseSets = SdaExchangeBaseSets;
	};
}}
