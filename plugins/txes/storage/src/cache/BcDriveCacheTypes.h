/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/BcDriveEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

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
		struct BcDriveHeightGroupingSerializer;

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

	/// Drive cache types.
	struct BcDriveCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<BcDriveCacheDescriptor, utils::ArrayHasher<Key>>;
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicBcDriveCacheView, BasicBcDriveCacheDelta, const Key&, state::BcDriveEntry>;

		using BaseSetDeltaPointers = BcDriveBaseSetDeltaPointers;
		using BaseSets = BcDriveBaseSets;
	};
}}
