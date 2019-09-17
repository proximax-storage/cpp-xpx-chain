/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/config/src/state/NetworkConfigEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/IdentifierSerializer.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicNetworkConfigCacheDelta;
		class BasicNetworkConfigCacheView;
		struct NetworkConfigBaseSetDeltaPointers;
		struct NetworkConfigBaseSets;
		class NetworkConfigCache;
		class NetworkConfigCacheDelta;
		class NetworkConfigCacheView;
		struct NetworkConfigEntryPrimarySerializer;
		class NetworkConfigPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a network config cache.
	struct NetworkConfigCacheDescriptor {
	public:
		static constexpr auto Name = "NetworkConfigCache";

	public:
		// key value types
		using KeyType = Height;
		using ValueType = state::NetworkConfigEntry;

		// cache types
		using CacheType = NetworkConfigCache;
		using CacheDeltaType = NetworkConfigCacheDelta;
		using CacheViewType = NetworkConfigCacheView;

		using Serializer = NetworkConfigEntryPrimarySerializer;
		using PatriciaTree = NetworkConfigPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.height();
		}
	};

	/// Network config cache types.
	struct NetworkConfigCacheTypes {
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicNetworkConfigCacheView, BasicNetworkConfigCacheDelta, const Height&, state::NetworkConfigEntry>;

		// region secondary descriptors

		struct HeightTypesDescriptor {
		public:
			using ValueType = Height;
			using KeyType = Height;
			using Serializer = IdentifierSerializer<HeightTypesDescriptor>;

			// cache types
			using CacheType = NetworkConfigCache;
			using CacheDeltaType = NetworkConfigCacheDelta;
			using CacheViewType = NetworkConfigCacheView;

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

		using PrimaryTypes = MutableUnorderedMapAdapter<NetworkConfigCacheDescriptor, utils::BaseValueHasher<Height>>;
		using HeightTypes = MutableOrderedMemorySetAdapter<HeightTypesDescriptor>;

		using BaseSetDeltaPointers = NetworkConfigBaseSetDeltaPointers;
		using BaseSets = NetworkConfigBaseSets;
	};
}}
