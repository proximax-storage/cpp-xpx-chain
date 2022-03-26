/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/LPEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/IdentifierSerializer.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicLPCacheDelta;
		class BasicLPCacheView;
		struct LPBaseSetDeltaPointers;
		struct LPBaseSets;
		class LPCache;
		class LPCacheDelta;
		class LPCacheView;
		struct LPEntryPrimarySerializer;
		class LPPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a drive cache.
	struct LPCacheDescriptor {
	public:
		static constexpr auto Name = "LPCache";

	public:
		// key value types
		using KeyType = MosaicId;
		using ValueType = state::LiquidityProviderEntry;

		// cache types
		using CacheType = LPCache;
		using CacheDeltaType = LPCacheDelta;
		using CacheViewType = LPCacheView;

		using Serializer = LPEntryPrimarySerializer;
		using PatriciaTree = LPPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.mosaicId();
		}
	};

	/// LP cache types.
	struct LPCacheTypes {
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicLPCacheView, BasicLPCacheDelta, const Key&, state::LiquidityProviderEntry>;

		// region secondary descriptors

		struct KeyTypesDescriptor {
		public:
			using ValueType = MosaicId;
			using KeyType = MosaicId;

			// cache types
			using CacheType = LPCache;
			using CacheDeltaType = LPCacheDelta;
			using CacheViewType = LPCacheView;

			using Serializer = UnorderedSetIdentifierSerializer<KeyTypesDescriptor>;

		public:
			static auto GetKeyFromValue(const ValueType& key) {
				return key;
			}
		};

		// endregion

		using PrimaryTypes = MutableUnorderedMapAdapter<LPCacheDescriptor, utils::BaseValueHasher<MosaicId>>;
		using KeyTypes = MutableUnorderedMemorySetAdapter<KeyTypesDescriptor, utils::BaseValueHasher<MosaicId>>;

		using BaseSetDeltaPointers = LPBaseSetDeltaPointers;
		using BaseSets = LPBaseSets;
	};
}}
