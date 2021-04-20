/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/DownloadChannelEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicDownloadCacheDelta;
		class BasicDownloadCacheView;
		struct DownloadBaseSetDeltaPointers;
		struct DownloadBaseSets;
		class DownloadCache;
		class DownloadCacheDelta;
		class DownloadCacheView;
		struct DownloadChannelEntryPrimarySerializer;
		class DownloadPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a download cache.
	struct DownloadCacheDescriptor {
	public:
		static constexpr auto Name = "DownloadChannelCache";

	public:
		// key value types
		using KeyType = Hash256;
		using ValueType = state::DownloadChannelEntry;

		// cache types
		using CacheType = DownloadCache;
		using CacheDeltaType = DownloadCacheDelta;
		using CacheViewType = DownloadCacheView;

		using Serializer = DownloadChannelEntryPrimarySerializer;
		using PatriciaTree = DownloadPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.id();
		}
	};

	/// Download cache types.
	struct DownloadCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<DownloadCacheDescriptor, utils::ArrayHasher<Hash256>>;
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicDownloadCacheView, BasicDownloadCacheDelta, const Hash256&, state::DownloadChannelEntry>;

		using BaseSetDeltaPointers = DownloadBaseSetDeltaPointers;
		using BaseSets = DownloadBaseSets;
	};
}}
