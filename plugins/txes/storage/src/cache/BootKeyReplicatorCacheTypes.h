/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/BootKeyReplicatorEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicBootKeyReplicatorCacheDelta;
		class BasicBootKeyReplicatorCacheView;
		struct BootKeyReplicatorBaseSetDeltaPointers;
		struct BootKeyReplicatorBaseSets;
		class BootKeyReplicatorCache;
		class BootKeyReplicatorCacheDelta;
		class BootKeyReplicatorCacheView;
		struct BootKeyReplicatorEntryPrimarySerializer;
		class BootKeyReplicatorPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a drive cache.
	struct BootKeyReplicatorCacheDescriptor {
	public:
		static constexpr auto Name = "BootKeyReplicatorCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::BootKeyReplicatorEntry;

		// cache types
		using CacheType = BootKeyReplicatorCache;
		using CacheDeltaType = BootKeyReplicatorCacheDelta;
		using CacheViewType = BootKeyReplicatorCacheView;

		using Serializer = BootKeyReplicatorEntryPrimarySerializer;
		using PatriciaTree = BootKeyReplicatorPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.nodeBootKey();
		}
	};

	/// Drive cache types.
	struct BootKeyReplicatorCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<BootKeyReplicatorCacheDescriptor, utils::ArrayHasher<Key>>;
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicBootKeyReplicatorCacheView, BasicBootKeyReplicatorCacheDelta, const Key&, state::BootKeyReplicatorEntry>;

		using BaseSetDeltaPointers = BootKeyReplicatorBaseSetDeltaPointers;
		using BaseSets = BootKeyReplicatorBaseSets;
	};
}}
