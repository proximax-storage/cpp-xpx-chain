/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/LiquidityProviderEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/IdentifierSerializer.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicLiquidityProviderCacheDelta;
		class BasicLiquidityProviderCacheView;
		struct LiquidityProviderBaseSetDeltaPointers;
		struct LiquidityProviderBaseSets;
		class LiquidityProviderCache;
		class LiquidityProviderCacheDelta;
		class LiquidityProviderCacheView;
		struct LiquidityProviderEntryPrimarySerializer;
		class LiquidityProviderPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a LiquidityProvider cache.
	struct LiquidityProviderCacheDescriptor {
	public:
		static constexpr auto Name = "LiquidityProviderCache";

	public:
		// key value types
		using KeyType = UnresolvedMosaicId;
		using ValueType = state::LiquidityProviderEntry;

		// cache types
		using CacheType = LiquidityProviderCache;
		using CacheDeltaType = LiquidityProviderCacheDelta;
		using CacheViewType = LiquidityProviderCacheView;

		using Serializer = LiquidityProviderEntryPrimarySerializer;
		using PatriciaTree = LiquidityProviderPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.mosaicId();
		}
	};

	/// LiquidityProvider cache types.
	struct LiquidityProviderCacheTypes {
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicLiquidityProviderCacheView, BasicLiquidityProviderCacheDelta, const UnresolvedMosaicId&, state::LiquidityProviderEntry>;

		// region secondary descriptors

		struct KeyTypesDescriptor {
		public:
			using ValueType = UnresolvedMosaicId;
			using KeyType = UnresolvedMosaicId;

			// cache types
			using CacheType = LiquidityProviderCache;
			using CacheDeltaType = LiquidityProviderCacheDelta;
			using CacheViewType = LiquidityProviderCacheView;

			using Serializer = UnorderedSetIdentifierSerializer<KeyTypesDescriptor>;

		public:
			static auto GetKeyFromValue(const ValueType& key) {
				return key;
			}
		};

		// endregion

		using PrimaryTypes = MutableUnorderedMapAdapter<LiquidityProviderCacheDescriptor, utils::BaseValueHasher<UnresolvedMosaicId>>;
		using KeyTypes = MutableUnorderedMemorySetAdapter<KeyTypesDescriptor, utils::BaseValueHasher<UnresolvedMosaicId>>;

		using BaseSetDeltaPointers = LiquidityProviderBaseSetDeltaPointers;
		using BaseSets = LiquidityProviderBaseSets;
	};
}}
