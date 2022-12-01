/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/DriveContractEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicDriveContractCacheDelta;
		class BasicDriveContractCacheView;
		struct DriveContractBaseSetDeltaPointers;
		struct DriveContractBaseSets;
		class DriveContractCache;
		class DriveContractCacheDelta;
		class DriveContractCacheView;
		struct DriveContractEntryPrimarySerializer;
		class DriveContractPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

    /// Describes a drivecontract cache.
	struct DriveContractCacheDescriptor {
        public:
            static constexpr auto Name = "DriveContractCache";

        public:
            // key value types
            using KeyType = Key;
            using ValueType = state::DriveContractEntry;

            // cache types
            using CacheType = DriveContractCache;
            using CacheDeltaType = DriveContractCacheDelta;
            using CacheViewType = DriveContractCacheView;

            using Serializer = DriveContractEntryPrimarySerializer;
            using PatriciaTree = DriveContractPatriciaTree;

        public:
            /// Gets the key corresponding to \a entry.
            static const auto& GetKeyFromValue(const ValueType& entry) {
                return entry.key();
            }
    };

    /// DriveContract cache types.
	struct DriveContractCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<DriveContractCacheDescriptor, utils::ArrayHasher<Key>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicDriveContractCacheView, BasicDriveContractCacheDelta, const Key&, state::DriveContractEntry>;

		using BaseSetDeltaPointers = DriveContractBaseSetDeltaPointers;
		using BaseSets = DriveContractBaseSets;
	};
}}