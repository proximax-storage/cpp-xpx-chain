/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/BlsKeysEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicBlsKeysCacheDelta;
		class BasicBlsKeysCacheView;
		struct BlsKeysBaseSetDeltaPointers;
		struct BlsKeysBaseSets;
		class BlsKeysCache;
		class BlsKeysCacheDelta;
		class BlsKeysCacheView;
		struct BlsKeysEntryPrimarySerializer;
		class BlsKeysPatriciaTree;
		struct BlsKeysHeightGroupingSerializer;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a BLS keys cache.
	struct BlsKeysCacheDescriptor {
	public:
		static constexpr auto Name = "BlsKeysCache";

	public:
		// key value types
		using KeyType = BLSPublicKey;
		using ValueType = state::BlsKeysEntry;

		// cache types
		using CacheType = BlsKeysCache;
		using CacheDeltaType = BlsKeysCacheDelta;
		using CacheViewType = BlsKeysCacheView;

		using Serializer = BlsKeysEntryPrimarySerializer;
		using PatriciaTree = BlsKeysPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.blsKey();
		}
	};

	/// BLS keys cache types.
	struct BlsKeysCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<BlsKeysCacheDescriptor, utils::ArrayHasher<BLSPublicKey>>;
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicBlsKeysCacheView, BasicBlsKeysCacheDelta, const BLSPublicKey&, state::BlsKeysEntry>;

		using BaseSetDeltaPointers = BlsKeysBaseSetDeltaPointers;
		using BaseSets = BlsKeysBaseSets;
	};
}}
