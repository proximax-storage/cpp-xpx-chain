/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/CommitteeEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/IdentifierSerializer.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicCommitteeCacheDelta;
		class BasicCommitteeCacheView;
		struct CommitteeBaseSetDeltaPointers;
		struct CommitteeBaseSets;
		class CommitteeCache;
		class CommitteeCacheDelta;
		class CommitteeCacheView;
		struct CommitteeEntryPrimarySerializer;
		class CommitteePatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a committee cache.
	struct CommitteeCacheDescriptor {
	public:
		static constexpr auto Name = "CommitteeCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::CommitteeEntry;

		// cache types
		using CacheType = CommitteeCache;
		using CacheDeltaType = CommitteeCacheDelta;
		using CacheViewType = CommitteeCacheView;

		using Serializer = CommitteeEntryPrimarySerializer;
		using PatriciaTree = CommitteePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// Committee cache types.
	struct CommitteeCacheTypes {
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicCommitteeCacheView, BasicCommitteeCacheDelta, const Key&, state::CommitteeEntry>;

		// region secondary descriptors

		struct KeyTypesDescriptor {
		public:
			using ValueType = Key;
			using KeyType = Key;

			// cache types
			using CacheType = CommitteeCache;
			using CacheDeltaType = CommitteeCacheDelta;
			using CacheViewType = CommitteeCacheView;

			using Serializer = UnorderedSetIdentifierSerializer<KeyTypesDescriptor>;

		public:
			static auto GetKeyFromValue(const ValueType& key) {
				return key;
			}
		};

		// endregion

		using PrimaryTypes = MutableUnorderedMapAdapter<CommitteeCacheDescriptor, utils::ArrayHasher<Key>>;
		using KeyTypes = MutableUnorderedMemorySetAdapter<KeyTypesDescriptor, utils::ArrayHasher<Key>>;

		using BaseSetDeltaPointers = CommitteeBaseSetDeltaPointers;
		using BaseSets = CommitteeBaseSets;
	};
}}
