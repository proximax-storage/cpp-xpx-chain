/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/ReputationEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicReputationCacheDelta;
		class BasicReputationCacheView;
		struct ReputationBaseSetDeltaPointers;
		struct ReputationBaseSets;
		class ReputationCache;
		class ReputationCacheDelta;
		class ReputationCacheView;
		struct ReputationEntryPrimarySerializer;
		class ReputationPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a reputation cache.
	struct ReputationCacheDescriptor {
	public:
		static constexpr auto Name = "ReputationCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::ReputationEntry;

		// cache types
		using CacheType = ReputationCache;
		using CacheDeltaType = ReputationCacheDelta;
		using CacheViewType = ReputationCacheView;

		using Serializer = ReputationEntryPrimarySerializer;
		using PatriciaTree = ReputationPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// Reputation cache types.
	struct ReputationCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<ReputationCacheDescriptor, utils::ArrayHasher<Key>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicReputationCacheView, BasicReputationCacheDelta, const Key&, state::ReputationEntry>;

		using BaseSetDeltaPointers = ReputationBaseSetDeltaPointers;
		using BaseSets = ReputationBaseSets;
	};
}}
