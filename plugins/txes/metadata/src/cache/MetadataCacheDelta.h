/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataBaseSets.h"
#include "MetadataCacheTypes.h"
#include "MetadataCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the metadata delta view.
	struct MetadataCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<MetadataCacheTypes::PrimaryTypes::BaseSetDeltaType, MetadataCacheDescriptor>;

	public:
		using Size = PrimaryMixins::Size;
		using Contains = PrimaryMixins::Contains;
		using PatriciaTreeDelta = PrimaryMixins::PatriciaTreeDelta;
		using MutableAccessor = PrimaryMixins::ConstAccessor;
		using ConstAccessor = PrimaryMixins::MutableAccessor;
		using DeltaElements = PrimaryMixins::DeltaElements;
		using Enable = PrimaryMixins::Enable;
		using Height = PrimaryMixins::Height;
	};

	/// Basic delta on top of the metadata cache.
	class BasicMetadataCacheDelta
			: public utils::MoveOnly
			, public MetadataCacheDeltaMixins::Size
			, public MetadataCacheDeltaMixins::Contains
			, public MetadataCacheDeltaMixins::PatriciaTreeDelta
			, public MetadataCacheDeltaMixins::ConstAccessor
			, public MetadataCacheDeltaMixins::MutableAccessor
			, public MetadataCacheDeltaMixins::DeltaElements
			, public MetadataCacheDeltaMixins::Enable
			, public MetadataCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = MetadataCacheTypes::CacheReadOnlyType;
		using CollectedIds = std::unordered_set<Hash256, utils::ArrayHasher<Hash256>>;

	public:
		/// Creates a delta around \a metadataSets.
		BasicMetadataCacheDelta(const MetadataCacheTypes::BaseSetDeltaPointers& metadataSets);

	public:
		using MetadataCacheDeltaMixins::ConstAccessor::find;
		using MetadataCacheDeltaMixins::MutableAccessor::find;

	public:
		/// Inserts the metadata \a ns into the cache.
		void insert(const state::MetadataEntry& metadata);

		/// Removes the metadata specified by its \a id from the cache.
		void remove(const Hash256& id);

		/// Prunes the metadata cache at \a height.
		CollectedIds prune(Height height);

	private:
		MetadataCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pMetadataById;
		MetadataCacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType m_pMetadataIdsByExpiryHeight;
	};

	/// Delta on top of the metadata cache.
	class MetadataCacheDelta : public ReadOnlyViewSupplier<BasicMetadataCacheDelta> {
	public:
		/// Creates a delta around \a metadataSets.
		MetadataCacheDelta(const MetadataCacheTypes::BaseSetDeltaPointers& metadataSets)
				: ReadOnlyViewSupplier(metadataSets)
		{}
	};
}}
