/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/ReplicatorEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicReplicatorCacheDelta;
		class BasicReplicatorCacheView;
		struct ReplicatorBaseSetDeltaPointers;
		struct ReplicatorBaseSets;
		class ReplicatorCache;
		class ReplicatorCacheDelta;
		class ReplicatorCacheView;
		struct ReplicatorEntryPrimarySerializer;
		class ReplicatorPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a drive cache.
	struct ReplicatorCacheDescriptor {
	public:
		static constexpr auto Name = "ReplicatorCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::ReplicatorEntry;

		// cache types
		using CacheType = ReplicatorCache;
		using CacheDeltaType = ReplicatorCacheDelta;
		using CacheViewType = ReplicatorCacheView;

		using Serializer = ReplicatorEntryPrimarySerializer;
		using PatriciaTree = ReplicatorPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// Drive cache types.
	struct ReplicatorCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<ReplicatorCacheDescriptor, utils::ArrayHasher<Key>>;
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicReplicatorCacheView, BasicReplicatorCacheDelta, const Key&, state::ReplicatorEntry>;

		using BaseSetDeltaPointers = ReplicatorBaseSetDeltaPointers;
		using BaseSets = ReplicatorBaseSets;
	};
}}
