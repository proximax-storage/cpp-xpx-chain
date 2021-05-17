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
		class BasicDownloadChannelCacheDelta;
		class BasicDownloadChannelCacheView;
		struct DownloadChannelBaseSetDeltaPointers;
		struct DownloadChannelBaseSets;
		class DownloadChannelCache;
		class DownloadChannelCacheDelta;
		class DownloadChannelCacheView;
		struct DownloadChannelEntryPrimarySerializer;
		class DownloadChannelPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a download cache.
	struct DownloadChannelCacheDescriptor {
	public:
		static constexpr auto Name = "DownloadChannelCache";

	public:
		// key value types
		using KeyType = Hash256;
		using ValueType = state::DownloadChannelEntry;

		// cache types
		using CacheType = DownloadChannelCache;
		using CacheDeltaType = DownloadChannelCacheDelta;
		using CacheViewType = DownloadChannelCacheView;

		using Serializer = DownloadChannelEntryPrimarySerializer;
		using PatriciaTree = DownloadChannelPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.id();
		}
	};

	/// Download cache types.
	struct DownloadChannelCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<DownloadChannelCacheDescriptor, utils::ArrayHasher<Hash256>>;
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicDownloadChannelCacheView, BasicDownloadChannelCacheDelta, const Hash256&, state::DownloadChannelEntry>;

		using BaseSetDeltaPointers = DownloadChannelBaseSetDeltaPointers;
		using BaseSets = DownloadChannelBaseSets;
	};
}}
