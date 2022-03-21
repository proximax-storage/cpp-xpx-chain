/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/QueueEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/IdentifierSerializer.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicQueueCacheDelta;
		class BasicQueueCacheView;
		struct QueueBaseSetDeltaPointers;
		struct QueueBaseSets;
		class QueueCache;
		class QueueCacheDelta;
		class QueueCacheView;
		struct QueueEntryPrimarySerializer;
		class QueuePatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a drive cache.
	struct QueueCacheDescriptor {
	public:
		static constexpr auto Name = "QueueCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::QueueEntry;

		// cache types
		using CacheType = QueueCache;
		using CacheDeltaType = QueueCacheDelta;
		using CacheViewType = QueueCacheView;

		using Serializer = QueueEntryPrimarySerializer;
		using PatriciaTree = QueuePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// Queue cache types.
	struct QueueCacheTypes {
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicQueueCacheView, BasicQueueCacheDelta, const Key&, state::QueueEntry>;

		// region secondary descriptors

		struct KeyTypesDescriptor {
		public:
			using ValueType = Key;
			using KeyType = Key;

			// cache types
			using CacheType = QueueCache;
			using CacheDeltaType = QueueCacheDelta;
			using CacheViewType = QueueCacheView;

			using Serializer = UnorderedSetIdentifierSerializer<KeyTypesDescriptor>;

		public:
			static auto GetKeyFromValue(const ValueType& key) {
				return key;
			}
		};

		// endregion

		using PrimaryTypes = MutableUnorderedMapAdapter<QueueCacheDescriptor, utils::ArrayHasher<Key>>;
		using KeyTypes = MutableUnorderedMemorySetAdapter<KeyTypesDescriptor, utils::ArrayHasher<Key>>;

		using BaseSetDeltaPointers = QueueBaseSetDeltaPointers;
		using BaseSets = QueueBaseSets;
	};
}}
