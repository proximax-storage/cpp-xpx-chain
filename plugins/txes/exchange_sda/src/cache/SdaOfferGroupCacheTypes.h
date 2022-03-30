/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/SdaOfferGroupEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicSdaOfferGroupCacheDelta;
		class BasicSdaOfferGroupCacheView;
		struct SdaOfferGroupBaseSetDeltaPointers;
		struct SdaOfferGroupBaseSets;
		class SdaOfferGroupCache;
		class SdaOfferGroupCacheDelta;
		class SdaOfferGroupCacheView;
		struct SdaOfferGroupEntryPrimarySerializer;
		struct SdaOfferGroupHeightGroupingSerializer;
		class SdaOfferGroupPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a SDA-SDA offer group cache.
	struct SdaOfferGroupCacheDescriptor {
	public:
		static constexpr auto Name = "SdaOfferGroupCache";

	public:
		// key value types
		using KeyType = Hash256;
		using ValueType = state::SdaOfferGroupEntry;

		// cache types
		using CacheType = SdaOfferGroupCache;
		using CacheDeltaType = SdaOfferGroupCacheDelta;
		using CacheViewType = SdaOfferGroupCacheView;

		using Serializer = SdaOfferGroupEntryPrimarySerializer;
		using PatriciaTree = SdaOfferGroupPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.groupHash();
		}
	};

	/// SDA-SDA Offer Group cache types.
	struct SdaOfferGroupCacheTypes {
	public:
		struct HeightGroupingTypesDescriptor {
		public:
			using KeyType = Height;
			using ValueType = utils::OrderedIdentifierGroup<Hash256, Height>;
			using Serializer = SdaOfferGroupHeightGroupingSerializer;

		public:
			static auto GetKeyFromValue(const ValueType& heightSdaOfferGroup) {
				return heightSdaOfferGroup.key();
			}
		};

		using PrimaryTypes = MutableUnorderedMapAdapter<SdaOfferGroupCacheDescriptor, utils::ArrayHasher<Hash256>>;
		using HeightGroupingTypes = MutableUnorderedMapAdapter<HeightGroupingTypesDescriptor, utils::BaseValueHasher<Height>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicSdaOfferGroupCacheView, BasicSdaOfferGroupCacheDelta, const Hash256&, state::SdaOfferGroupEntry>;

		using BaseSetDeltaPointers = SdaOfferGroupBaseSetDeltaPointers;
		using BaseSets = SdaOfferGroupBaseSets;
	};
}}
