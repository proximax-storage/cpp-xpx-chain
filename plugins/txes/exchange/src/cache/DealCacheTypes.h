/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/DealEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/ShortHash.h"

namespace catapult {
	namespace cache {
		class BasicDealCacheDelta;
		class BasicDealCacheView;
		struct DealBaseSetDeltaPointers;
		struct DealBaseSets;
		class DealCache;
		class DealCacheDelta;
		class DealCacheView;
		struct DealEntryPrimarySerializer;
		class DealPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a deal cache.
	struct DealCacheDescriptor {
	public:
		static constexpr auto Name = "DealCache";

	public:
		// key value types
		using KeyType = utils::ShortHash;
		using ValueType = state::DealEntry;

		// cache types
		using CacheType = DealCache;
		using CacheDeltaType = DealCacheDelta;
		using CacheViewType = DealCacheView;

		using Serializer = DealEntryPrimarySerializer;
		using PatriciaTree = DealPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.transactionHash();
		}
	};

	/// Deal cache types.
	struct DealCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<DealCacheDescriptor, utils::BaseValueHasher<utils::ShortHash>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicDealCacheView, BasicDealCacheDelta, const utils::ShortHash&, state::DealEntry>;

		using BaseSetDeltaPointers = DealBaseSetDeltaPointers;
		using BaseSets = DealBaseSets;
	};
}}
