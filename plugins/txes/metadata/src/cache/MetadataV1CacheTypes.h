/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "plugins/txes/metadata/src/state/MetadataV1Entry.h"
#include "plugins/txes/metadata/src/state/MetadataV1Serializer.h"
#include "catapult/cache/CacheDatabaseMixin.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicMetadataV1CacheDelta;
		class BasicMetadataV1CacheView;
		struct MetadataV1BaseSetDeltaPointers;
		struct MetadataV1BaseSets;
		class MetadataV1Cache;
		class MetadataV1CacheDelta;
		class MetadataV1CacheView;
		struct MetadataV1HeightGroupingSerializer;
		class MetadataV1PatriciaTree;
		struct MetadataV1PrimarySerializer;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a metadata cache.
	struct MetadataV1CacheDescriptor {
	public:
		static constexpr auto Name = "MetadataCache";

	public:
		// key value types
		using KeyType = Hash256;
		using ValueType = state::MetadataV1Entry;

		// cache types
		using CacheType = MetadataV1Cache;
		using CacheDeltaType = MetadataV1CacheDelta;
		using CacheViewType = MetadataV1CacheView;

		using Serializer = MetadataV1PrimarySerializer;
		using PatriciaTree = MetadataV1PatriciaTree;

	public:
		/// Gets the metadataId corresponding to \a metadata.
		static auto GetKeyFromValue(const ValueType& metadata) {
			return metadata.metadataId();
		}
	};

	/// Metadata cache types.
	struct MetadataV1CacheTypes {
	public:
		using CacheReadOnlyType = ReadOnlyArtifactCache<
			BasicMetadataV1CacheView,
			BasicMetadataV1CacheDelta,
			const Hash256&,
			state::MetadataV1Entry>;
	// region secondary descriptors

	public:
		struct HeightGroupingTypesDescriptor {
		public:
			static constexpr auto Name = "MetadataCache";
			using KeyType = Height;
			using ValueType = utils::UnorderedIdentifierGroup<Hash256, Height, utils::ArrayHasher<Hash256>>;
			using Serializer = MetadataV1HeightGroupingSerializer;

		public:
			static auto GetKeyFromValue(const ValueType& heightMetadatas) {
				return heightMetadatas.key();
			}
		};

	// endregion

	public:
		using PrimaryTypes = MutableUnorderedMapAdapter<MetadataV1CacheDescriptor, utils::ArrayHasher<Hash256>>;
		using HeightGroupingTypes = MutableUnorderedMapAdapter<HeightGroupingTypesDescriptor, utils::BaseValueHasher<Height>>;

	public:
		using BaseSetDeltaPointers = MetadataV1BaseSetDeltaPointers;
		using BaseSets = MetadataV1BaseSets;
	};
}}
