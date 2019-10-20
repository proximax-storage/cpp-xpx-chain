/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/OfferDeadlineEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicOfferDeadlineCacheDelta;
		class BasicOfferDeadlineCacheView;
		struct OfferDeadlineBaseSetDeltaPointers;
		struct OfferDeadlineBaseSets;
		class OfferDeadlineCache;
		class OfferDeadlineCacheDelta;
		class OfferDeadlineCacheView;
		struct OfferDeadlineEntryPrimarySerializer;
		class OfferDeadlinePatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a offer deadline cache.
	struct OfferDeadlineCacheDescriptor {
	public:
		static constexpr auto Name = "OfferDeadlineCache";

	public:
		// key value types
		using KeyType = Height;
		using ValueType = state::OfferDeadlineEntry;

		// cache types
		using CacheType = OfferDeadlineCache;
		using CacheDeltaType = OfferDeadlineCacheDelta;
		using CacheViewType = OfferDeadlineCacheView;

		using Serializer = OfferDeadlineEntryPrimarySerializer;
		using PatriciaTree = OfferDeadlinePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.height();
		}
	};

	/// OfferDeadline cache types.
	struct OfferDeadlineCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<OfferDeadlineCacheDescriptor, utils::BaseValueHasher<Height>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicOfferDeadlineCacheView, BasicOfferDeadlineCacheDelta, const Height&, state::OfferDeadlineEntry>;

		using BaseSetDeltaPointers = OfferDeadlineBaseSetDeltaPointers;
		using BaseSets = OfferDeadlineBaseSets;
	};
}}
