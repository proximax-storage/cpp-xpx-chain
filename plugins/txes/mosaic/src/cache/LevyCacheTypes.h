/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once

#include "src/catapult/utils/IdentifierGroup.h"
#include "src/state/LevyEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicLevyCacheDelta;
		class BasicLevyCacheView;
		struct LevyBaseSetDeltaPointers;
		struct LevyBaseSets;
		class LevyCache;
		class LevyCacheDelta;
		class LevyCacheView;
		struct LevyEntryPrimarySerializer;
		class LevyPatriciaTree;
		struct LevyHeightGroupingSerializer;
		
		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {
		
	/// Describes a catapult upgrade cache.
	struct LevyCacheDescriptor {
	public:
		static constexpr auto Name = "LevyCache";
		
	public:
		// key value types
		using KeyType = MosaicId;
		using ValueType = state::LevyEntry;
			
		// cache types
		using CacheType = LevyCache;
		using CacheDeltaType = LevyCacheDelta;
		using CacheViewType = LevyCacheView;
			
		using Serializer = LevyEntryPrimarySerializer;
		using PatriciaTree = LevyPatriciaTree;
		
	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.mosaicId();
		}
	};
		
	/// Levy cache types.
	struct LevyCacheTypes {
		struct HeightGroupingTypesDescriptor {
		public:
			using KeyType = Height;
			using ValueType = utils::IdentifierGroup<MosaicId, Height, utils::BaseValueHasher<MosaicId>>;
			using Serializer = LevyHeightGroupingSerializer;
		
		public:
			static auto GetKeyFromValue(const ValueType& identifierGroup) {
				return identifierGroup.key();
			}
		};
		
		// important, when using Key as type, use ArrayHasher and not BaseValueHasher
		// data type Key is defined as a utils::ByteArray
		// BaseValueHasher is for utils::BaseValue type
		using PrimaryTypes = MutableUnorderedMapAdapter<LevyCacheDescriptor, utils::BaseValueHasher<MosaicId>>;
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicLevyCacheView, BasicLevyCacheDelta, const MosaicId&, state::LevyEntry>;
		using HeightGroupingTypes = MutableUnorderedMapAdapter<HeightGroupingTypesDescriptor, utils::BaseValueHasher<Height>>;
		
		using BaseSetDeltaPointers = LevyBaseSetDeltaPointers;
		using BaseSets = LevyBaseSets;
	};
}}
