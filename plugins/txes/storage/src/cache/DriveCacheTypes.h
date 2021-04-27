/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/DriveEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicDriveCacheDelta;
		class BasicDriveCacheView;
		struct DriveBaseSetDeltaPointers;
		struct DriveBaseSets;
		class DriveCache;
		class DriveCacheDelta;
		class DriveCacheView;
		struct DriveEntryPrimarySerializer;
		class DrivePatriciaTree;
		struct DriveHeightGroupingSerializer;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a drive cache.
	struct DriveCacheDescriptor {
	public:
		static constexpr auto Name = "DriveCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::DriveEntry;

		// cache types
		using CacheType = DriveCache;
		using CacheDeltaType = DriveCacheDelta;
		using CacheViewType = DriveCacheView;

		using Serializer = DriveEntryPrimarySerializer;
		using PatriciaTree = DrivePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// Drive cache types.
	struct DriveCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<DriveCacheDescriptor, utils::ArrayHasher<Key>>;
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicDriveCacheView, BasicDriveCacheDelta, const Key&, state::DriveEntry>;

		using BaseSetDeltaPointers = DriveBaseSetDeltaPointers;
		using BaseSets = DriveBaseSets;
	};
}}
