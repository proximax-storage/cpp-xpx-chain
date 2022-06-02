/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/PriorityQueueEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicPriorityQueueCacheDelta;
		class BasicPriorityQueueCacheView;
		struct PriorityQueueBaseSetDeltaPointers;
		struct PriorityQueueBaseSets;
		class PriorityQueueCache;
		class PriorityQueueCacheDelta;
		class PriorityQueueCacheView;
		struct PriorityQueueEntryPrimarySerializer;
		class PriorityQueuePatriciaTree;
		struct PriorityQueueHeightGroupingSerializer;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a priority queue cache.
	struct PriorityQueueCacheDescriptor {
	public:
		static constexpr auto Name = "PriorityQueueCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::PriorityQueueEntry;

		// cache types
		using CacheType = PriorityQueueCache;
		using CacheDeltaType = PriorityQueueCacheDelta;
		using CacheViewType = PriorityQueueCacheView;

		using Serializer = PriorityQueueEntryPrimarySerializer;
		using PatriciaTree = PriorityQueuePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// Priority queue cache types.
	struct PriorityQueueCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<PriorityQueueCacheDescriptor, utils::ArrayHasher<Key>>;
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicPriorityQueueCacheView, BasicPriorityQueueCacheDelta, const Key&, state::PriorityQueueEntry>;

		using BaseSetDeltaPointers = PriorityQueueBaseSetDeltaPointers;
		using BaseSets = PriorityQueueBaseSets;
	};
}}
