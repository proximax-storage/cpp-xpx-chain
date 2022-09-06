/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#pragma once
#include "../state/GlobalEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		struct GlobalStoreBaseSetDeltaPointers;
		struct GlobalStoreBaseSets;
		class GlobalStoreCache;
		class GlobalStoreCacheDelta;
		class GlobalStoreCacheView;
		class GlobalStorePatriciaTree;
		struct GlobalStorePrimarySerializer;
		class BasicGlobalStoreCacheDelta;
		class BasicGlobalStoreCacheView;

		template<typename TCache, typename TCacheDelta, typename TCacheKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes an global store cache.
	struct GlobalStoreCacheDescriptor {
	public:
		static constexpr auto Name = "GlobalStoreCache";

	public:
		// key value types
		using KeyType = Hash256;
		using ValueType = state::GlobalEntry;

		// cache types
		using CacheType = GlobalStoreCache;
		using CacheDeltaType = GlobalStoreCacheDelta;
		using CacheViewType = GlobalStoreCacheView;

		using Serializer = GlobalStorePrimarySerializer;
		using PatriciaTree = GlobalStorePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.GetKey();
		}
	};

	/// Global Store cache types.
	struct GlobalStoreCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<GlobalStoreCacheDescriptor, utils::ArrayHasher<Hash256>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<
			BasicGlobalStoreCacheView,
			BasicGlobalStoreCacheDelta,
			Hash256,
			state::GlobalEntry>;

		using BaseSetDeltaPointers = GlobalStoreBaseSetDeltaPointers;
		using BaseSets = GlobalStoreBaseSets;
	};
}}
