/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/FileEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicFileCacheDelta;
		class BasicFileCacheView;
		struct FileBaseSetDeltaPointers;
		struct FileBaseSets;
		class FileCache;
		class FileCacheDelta;
		class FileCacheView;
		struct FileEntryPrimarySerializer;
		class FilePatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a file cache.
	struct FileCacheDescriptor {
	public:
		static constexpr auto Name = "FileCache";

	public:
		// key value types
		using KeyType = state::DriveFileKey;
		using ValueType = state::FileEntry;

		// cache types
		using CacheType = FileCache;
		using CacheDeltaType = FileCacheDelta;
		using CacheViewType = FileCacheView;

		using Serializer = FileEntryPrimarySerializer;
		using PatriciaTree = FilePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// File cache types.
	struct FileCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<FileCacheDescriptor, utils::ArrayHasher<state::DriveFileKey>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicFileCacheView, BasicFileCacheDelta, const state::DriveFileKey&, state::FileEntry>;

		using BaseSetDeltaPointers = FileBaseSetDeltaPointers;
		using BaseSets = FileBaseSets;
	};
}}
