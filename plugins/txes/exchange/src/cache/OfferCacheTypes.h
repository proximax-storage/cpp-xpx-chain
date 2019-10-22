/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/OfferEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"
#include "catapult/utils/ShortHash.h"

namespace catapult {
	namespace cache {
		class BasicOfferCacheDelta;
		class BasicOfferCacheView;
		struct OfferBaseSetDeltaPointers;
		struct OfferBaseSets;
		class OfferCache;
		class OfferCacheDelta;
		class OfferCacheView;
		struct OfferEntryPrimarySerializer;
		struct OfferHeightGroupingSerializer;
		class OfferPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a offer cache.
	struct OfferCacheDescriptor {
	public:
		static constexpr auto Name = "OfferCache";

	public:
		// key value types
		using KeyType = utils::ShortHash;
		using ValueType = state::OfferEntry;

		// cache types
		using CacheType = OfferCache;
		using CacheDeltaType = OfferCacheDelta;
		using CacheViewType = OfferCacheView;

		using Serializer = OfferEntryPrimarySerializer;
		using PatriciaTree = OfferPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.transactionHash();
		}
	};

	/// Offer cache types.
	struct OfferCacheTypes {
	public:
		struct HeightGroupingTypesDescriptor {
		public:
			using KeyType = Height;
			using ValueType = utils::IdentifierGroup<utils::ShortHash, Height, utils::BaseValueHasher<utils::ShortHash>>;
			using Serializer = OfferHeightGroupingSerializer;

		public:
			static auto GetKeyFromValue(const ValueType& heightOffer) {
				return heightOffer.key();
			}
		};

		using PrimaryTypes = MutableUnorderedMapAdapter<OfferCacheDescriptor, utils::BaseValueHasher<utils::ShortHash>>;
		using HeightGroupingTypes = MutableUnorderedMapAdapter<HeightGroupingTypesDescriptor, utils::BaseValueHasher<Height>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicOfferCacheView, BasicOfferCacheDelta, const utils::ShortHash&, state::OfferEntry>;

		using BaseSetDeltaPointers = OfferBaseSetDeltaPointers;
		using BaseSets = OfferBaseSets;
	};
}}
