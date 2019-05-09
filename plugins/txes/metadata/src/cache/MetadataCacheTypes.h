/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/catapult/types.h"
#include "plugins/txes/metadata/src/state/MetadataEntry.h"
#include "plugins/txes/metadata/src/state/MetadataSerializer.h"
#include "catapult/cache/CacheDatabaseMixin.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicMetadataCacheDelta;
		class BasicMetadataCacheView;
		struct MetadataBaseSetDeltaPointers;
		struct MetadataBaseSets;
		class MetadataCache;
		class MetadataCacheDelta;
		class MetadataCacheView;
		struct MetadataHeightGroupingSerializer;
		class MetadataPatriciaTree;
		struct MetadataPrimarySerializer;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a metadata cache.
	struct MetadataCacheDescriptor {
	public:
		static constexpr auto Name = "MetadataCache";

	public:
		// key value types
		using KeyType = Hash256;
		using ValueType = state::MetadataEntry;

		// cache types
		using CacheType = MetadataCache;
		using CacheDeltaType = MetadataCacheDelta;
		using CacheViewType = MetadataCacheView;

		using Serializer = MetadataPrimarySerializer;
		using PatriciaTree = MetadataPatriciaTree;

	public:
		/// Gets the metadataId corresponding to \a metadata.
		static auto GetKeyFromValue(const ValueType& metadata) {
			return metadata.metadataId();
		}
	};

	/// Metadata cache types.
	struct MetadataCacheTypes {
	public:
		using CacheReadOnlyType = ReadOnlyArtifactCache<
			BasicMetadataCacheView,
			BasicMetadataCacheDelta,
			const Hash256&,
			state::MetadataEntry>;
	// region secondary descriptors

	public:
		struct HeightGroupingTypesDescriptor {
		public:
			using KeyType = Height;
			using ValueType = utils::IdentifierGroup<Hash256, Height, utils::ArrayHasher<Hash256>>;
			using Serializer = MetadataHeightGroupingSerializer;

		public:
			static auto GetKeyFromValue(const ValueType& heightMetadatas) {
				return heightMetadatas.key();
			}
		};

	// endregion

	public:
		using PrimaryTypes = MutableUnorderedMapAdapter<MetadataCacheDescriptor, utils::ArrayHasher<Hash256>>;
		using HeightGroupingTypes = MutableUnorderedMapAdapter<HeightGroupingTypesDescriptor, utils::BaseValueHasher<Height>>;

	public:
		using BaseSetDeltaPointers = MetadataBaseSetDeltaPointers;
		using BaseSets = MetadataBaseSets;
	};
}}
