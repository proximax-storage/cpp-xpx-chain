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
		class BasicBuyOfferCacheDelta;
		class BasicBuyOfferCacheView;
		struct BuyOfferBaseSetDeltaPointers;
		struct BuyOfferBaseSets;
		class BuyOfferCache;
		class BuyOfferCacheDelta;
		class BuyOfferCacheView;
		struct BuyOfferEntryPrimarySerializer;
		class BuyOfferPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a buy offer cache.
	struct BuyOfferCacheDescriptor {
	public:
		static constexpr auto Name = "BuyOfferCache";

	public:
		// key value types
		using KeyType = utils::ShortHash;
		using ValueType = state::OfferEntry;

		// cache types
		using CacheType = BuyOfferCache;
		using CacheDeltaType = BuyOfferCacheDelta;
		using CacheViewType = BuyOfferCacheView;

		using Serializer = BuyOfferEntryPrimarySerializer;
		using PatriciaTree = BuyOfferPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.transactionHash();
		}
	};

	/// BuyOffer cache types.
	struct BuyOfferCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<BuyOfferCacheDescriptor, utils::BaseValueHasher<utils::ShortHash>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicBuyOfferCacheView, BasicBuyOfferCacheDelta, const utils::ShortHash&, state::OfferEntry>;

		using BaseSetDeltaPointers = BuyOfferBaseSetDeltaPointers;
		using BaseSets = BuyOfferBaseSets;
	};
}}
