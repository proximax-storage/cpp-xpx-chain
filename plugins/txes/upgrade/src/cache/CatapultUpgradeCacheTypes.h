/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/CatapultUpgradeEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicCatapultUpgradeCacheDelta;
		class BasicCatapultUpgradeCacheView;
		struct CatapultUpgradeBaseSetDeltaPointers;
		struct CatapultUpgradeBaseSets;
		class CatapultUpgradeCache;
		class CatapultUpgradeCacheDelta;
		class CatapultUpgradeCacheView;
		struct CatapultUpgradeEntryPrimarySerializer;
		class CatapultUpgradePatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a catapult upgrade cache.
	struct CatapultUpgradeCacheDescriptor {
	public:
		static constexpr auto Name = "CatapultUpgradeCache";

	public:
		// key value types
		using KeyType = Height;
		using ValueType = state::CatapultUpgradeEntry;

		// cache types
		using CacheType = CatapultUpgradeCache;
		using CacheDeltaType = CatapultUpgradeCacheDelta;
		using CacheViewType = CatapultUpgradeCacheView;

		using Serializer = CatapultUpgradeEntryPrimarySerializer;
		using PatriciaTree = CatapultUpgradePatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.height();
		}
	};

	/// Catapult upgrade cache types.
	struct CatapultUpgradeCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<CatapultUpgradeCacheDescriptor, utils::BaseValueHasher<Height>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicCatapultUpgradeCacheView, BasicCatapultUpgradeCacheDelta, const Height&, state::CatapultUpgradeEntry>;

		using BaseSetDeltaPointers = CatapultUpgradeBaseSetDeltaPointers;
		using BaseSets = CatapultUpgradeBaseSets;
	};
}}
