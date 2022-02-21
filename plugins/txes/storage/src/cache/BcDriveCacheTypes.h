/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/BcDriveEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/IdentifierSerializer.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicBcDriveCacheDelta;
		class BasicBcDriveCacheView;
		struct BcDriveBaseSetDeltaPointers;
		struct BcDriveBaseSets;
		class BcDriveCache;
		class BcDriveCacheDelta;
		class BcDriveCacheView;
		struct BcDriveEntryPrimarySerializer;
		class BcDrivePatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a drive cache.
	struct BcDriveCacheDescriptor {
	public:
		static constexpr auto Name = "BcDriveCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::BcDriveEntry;

		// cache types
		using CacheType = BcDriveCache;
		using CacheDeltaType = BcDriveCacheDelta;
		using CacheViewType = BcDriveCacheView;

		using Serializer = BcDriveEntryPrimarySerializer;
		using PatriciaTree = BcDrivePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// BcDrive cache types.
	struct BcDriveCacheTypes {
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicBcDriveCacheView, BasicBcDriveCacheDelta, const Key&, state::BcDriveEntry>;

		// region secondary descriptors

		struct KeyTypesDescriptor {
		public:
			using ValueType = Key;
			using KeyType = Key;

			// cache types
			using CacheType = BcDriveCache;
			using CacheDeltaType = BcDriveCacheDelta;
			using CacheViewType = BcDriveCacheView;

			using Serializer = UnorderedSetIdentifierSerializer<KeyTypesDescriptor>;

		public:
			static auto GetKeyFromValue(const ValueType& key) {
				return key;
			}
		};

		// endregion

		using PrimaryTypes = MutableUnorderedMapAdapter<BcDriveCacheDescriptor, utils::ArrayHasher<Key>>;
		using KeyTypes = MutableUnorderedMemorySetAdapter<KeyTypesDescriptor, utils::ArrayHasher<Key>>;

		using BaseSetDeltaPointers = BcDriveBaseSetDeltaPointers;
		using BaseSets = BcDriveBaseSets;
	};
}}
