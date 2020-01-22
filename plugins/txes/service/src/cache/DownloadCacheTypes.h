/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/DownloadEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicDownloadCacheDelta;
		class BasicDownloadCacheView;
		struct DownloadBaseSetDeltaPointers;
		struct DownloadBaseSets;
		class DownloadCache;
		class DownloadCacheDelta;
		class DownloadCacheView;
		struct DownloadEntryPrimarySerializer;
		class DownloadPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a download cache.
	struct DownloadCacheDescriptor {
	public:
		static constexpr auto Name = "DownloadCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::DownloadEntry;

		// cache types
		using CacheType = DownloadCache;
		using CacheDeltaType = DownloadCacheDelta;
		using CacheViewType = DownloadCacheView;

		using Serializer = DownloadEntryPrimarySerializer;
		using PatriciaTree = DownloadPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.driveKey();
		}
	};

	/// Catapult upgrade cache types.
	struct DownloadCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<DownloadCacheDescriptor, utils::ArrayHasher<Key>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicDownloadCacheView, BasicDownloadCacheDelta, const Key&, state::DownloadEntry>;

		using BaseSetDeltaPointers = DownloadBaseSetDeltaPointers;
		using BaseSets = DownloadBaseSets;
	};
}}
