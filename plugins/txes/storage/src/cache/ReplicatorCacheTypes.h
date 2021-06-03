/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/ReplicatorEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/IdentifierSerializer.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
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

	/// Describes a replicator cache.
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

	/// Replicator cache types.
	struct ReplicatorCacheTypes {
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicReplicatorCacheView, BasicReplicatorCacheDelta, const Key&, state::ReplicatorEntry>;

		// region secondary descriptors

		struct KeyTypesDescriptor {
		public:
			using ValueType = Key;
			using KeyType = Key;

			// cache types
			using CacheType = ReplicatorCache;
			using CacheDeltaType = ReplicatorCacheDelta;
			using CacheViewType = ReplicatorCacheView;

			using Serializer = UnorderedSetIdentifierSerializer<KeyTypesDescriptor>;

		public:
			static auto GetKeyFromValue(const ValueType& key) {
				return key;
			}
		};

		// endregion

		using PrimaryTypes = MutableUnorderedMapAdapter<ReplicatorCacheDescriptor, utils::ArrayHasher<Key>>;
		using KeyTypes = MutableUnorderedMemorySetAdapter<KeyTypesDescriptor, utils::ArrayHasher<Key>>;

		using BaseSetDeltaPointers = ReplicatorBaseSetDeltaPointers;
		using BaseSets = ReplicatorBaseSets;
	};
}}
