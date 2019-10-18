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
#include "catapult/utils/ShortHash.h"

namespace catapult {
	namespace cache {
		class BasicSellOfferCacheDelta;
		class BasicSellOfferCacheView;
		struct SellOfferBaseSetDeltaPointers;
		struct SellOfferBaseSets;
		class SellOfferCache;
		class SellOfferCacheDelta;
		class SellOfferCacheView;
		struct SellOfferEntryPrimarySerializer;
		class SellOfferPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a sell offer cache.
	struct SellOfferCacheDescriptor {
	public:
		static constexpr auto Name = "SellOfferCache";

	public:
		// key value types
		using KeyType = utils::ShortHash;
		using ValueType = state::OfferEntry;

		// cache types
		using CacheType = SellOfferCache;
		using CacheDeltaType = SellOfferCacheDelta;
		using CacheViewType = SellOfferCacheView;

		using Serializer = SellOfferEntryPrimarySerializer;
		using PatriciaTree = SellOfferPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.transactionHash();
		}
	};

	/// SellOffer cache types.
	struct SellOfferCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<SellOfferCacheDescriptor, utils::BaseValueHasher<utils::ShortHash>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicSellOfferCacheView, BasicSellOfferCacheDelta, const utils::ShortHash&, state::OfferEntry>;

		using BaseSetDeltaPointers = SellOfferBaseSetDeltaPointers;
		using BaseSets = SellOfferBaseSets;
	};
}}
