/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/config/src/state/CatapultConfigEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/IdentifierSerializer.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicCatapultConfigCacheDelta;
		class BasicCatapultConfigCacheView;
		struct CatapultConfigBaseSetDeltaPointers;
		struct CatapultConfigBaseSets;
		class CatapultConfigCache;
		class CatapultConfigCacheDelta;
		class CatapultConfigCacheView;
		struct CatapultConfigEntryPrimarySerializer;
		class CatapultConfigPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a catapult config cache.
	struct CatapultConfigCacheDescriptor {
	public:
		static constexpr auto Name = "CatapultConfigCache";

	public:
		// key value types
		using KeyType = Height;
		using ValueType = state::CatapultConfigEntry;

		// cache types
		using CacheType = CatapultConfigCache;
		using CacheDeltaType = CatapultConfigCacheDelta;
		using CacheViewType = CatapultConfigCacheView;

		using Serializer = CatapultConfigEntryPrimarySerializer;
		using PatriciaTree = CatapultConfigPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.height();
		}
	};

	/// Catapult config cache types.
	struct CatapultConfigCacheTypes {
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicCatapultConfigCacheView, BasicCatapultConfigCacheDelta, const Height&, state::CatapultConfigEntry>;

		// region secondary descriptors

		struct HeightTypesDescriptor {
		public:
			using ValueType = Height;
			using KeyType = Height;
			using Serializer = IdentifierSerializer<HeightTypesDescriptor>;

			// cache types
			using CacheType = CatapultConfigCache;
			using CacheDeltaType = CatapultConfigCacheDelta;
			using CacheViewType = CatapultConfigCacheView;

		public:
			static auto GetKeyFromValue(const ValueType& height) {
				return height;
			}

			/// Converts \a height to pruning boundary.
			static uint64_t KeyToBoundary(const ValueType& height) {
				return height.unwrap();
			}
		};

		// endregion

		using PrimaryTypes = MutableUnorderedMapAdapter<CatapultConfigCacheDescriptor, utils::BaseValueHasher<Height>>;
		using HeightTypes = MutableOrderedMemorySetAdapter<HeightTypesDescriptor>;

		using BaseSetDeltaPointers = CatapultConfigBaseSetDeltaPointers;
		using BaseSets = CatapultConfigBaseSets;
	};
}}
