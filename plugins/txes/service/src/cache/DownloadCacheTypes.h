/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/DownloadEntry.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheTypes.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"

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
	}
}

namespace catapult { namespace cache {

	/// Describes a download cache.
	struct DownloadCacheDescriptor {
	public:
		static constexpr auto Name = "DownloadCache";

	public:
		// key value types
		using KeyType = Hash256;
		using ValueType = state::DownloadEntry;

		// cache types
		using CacheType = DownloadCache;
		using CacheDeltaType = DownloadCacheDelta;
		using CacheViewType = DownloadCacheView;

		using Serializer = DownloadEntryPrimarySerializer;
		using PatriciaTree = DownloadPatriciaTree;

	public:
		/// Gets the key corresponding to \a downloadEntry.
		static const auto& GetKeyFromValue(const ValueType& downloadEntry) {
			return downloadEntry.OperationToken;
		}
	};

	/// Download cache types.
	struct DownloadCacheTypes : public LockInfoCacheTypes<DownloadCacheDescriptor> {
		using CacheReadOnlyType = ReadOnlyArtifactCache<
			BasicDownloadCacheView,
			BasicDownloadCacheDelta,
			Hash256,
			state::DownloadEntry>;

		using BaseSetDeltaPointers = DownloadBaseSetDeltaPointers;
		using BaseSets = DownloadBaseSets;
	};
}}
