/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataV1BaseSets.h"
#include "MetadataV1CacheTypes.h"
#include "MetadataV1CacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the metadata delta view.
	struct MetadataV1CacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<MetadataV1CacheTypes::PrimaryTypes::BaseSetDeltaType, MetadataV1CacheDescriptor>;

	public:
		using Size = PrimaryMixins::Size;
		using Contains = PrimaryMixins::Contains;
		using PatriciaTreeDelta = PrimaryMixins::PatriciaTreeDelta;
		using MutableAccessor = PrimaryMixins::MutableAccessor;
		using ConstAccessor = PrimaryMixins::ConstAccessor;
		using DeltaElements = PrimaryMixins::DeltaElements;
		using PrivateAccessor = BroadMutableAccessorMixin<
				MetadataV1CacheTypes::PrimaryTypes::BaseSetDeltaType,
				MetadataV1CacheDescriptor,
				detail::NoOpAdapter<MetadataV1CacheDescriptor::ValueType>,
				MetadataV1CacheTypes::HeightGroupingTypes::BaseSetDeltaType,
				MetadataV1CacheTypes::HeightGroupingTypesDescriptor,
				detail::NoOpAdapter<MetadataV1CacheTypes::HeightGroupingTypesDescriptor::ValueType>>;
		using Enable = PrimaryMixins::Enable;
		using BroadIteration = BroadIterationMixin<MetadataV1CacheTypes::PrimaryTypes::BaseSetDeltaType, MetadataV1CacheTypes::HeightGroupingTypes::BaseSetDeltaType>;
		using Height = PrimaryMixins::Height;
	};

	/// Basic delta on top of the metadata cache.
	class BasicMetadataV1CacheDelta
			: public utils::MoveOnly
			, public MetadataV1CacheDeltaMixins::Size
			, public MetadataV1CacheDeltaMixins::Contains
			, public MetadataV1CacheDeltaMixins::PatriciaTreeDelta
			, public MetadataV1CacheDeltaMixins::ConstAccessor
			, public MetadataV1CacheDeltaMixins::MutableAccessor
			, public MetadataV1CacheDeltaMixins::DeltaElements
			, public MetadataV1CacheDeltaMixins::Enable
			, public MetadataV1CacheDeltaMixins::PrivateAccessor
			, public MetadataV1CacheDeltaMixins::Height
			, public MetadataV1CacheDeltaMixins::BroadIteration{
	public:
		using ReadOnlyView = MetadataV1CacheTypes::CacheReadOnlyType;
		using CollectedIds = std::unordered_set<Hash256, utils::ArrayHasher<Hash256>>;

	public:
		/// Creates a delta around \a metadataSets.
		BasicMetadataV1CacheDelta(const MetadataV1CacheTypes::BaseSetDeltaPointers& metadataSets);

	public:
		using MetadataV1CacheDeltaMixins::ConstAccessor::find;
		using MetadataV1CacheDeltaMixins::MutableAccessor::find;

	public:
		/// Inserts the metadata \a ns into the cache.
		void insert(const state::MetadataV1Entry& metadata);

		/// Removes the metadata specified by its \a id from the cache.
		void remove(const Hash256& id);

		/// Prunes the metadata cache at \a height.
		CollectedIds prune(Height height);

	private:
		MetadataV1CacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pMetadataById;
		MetadataV1CacheTypes::HeightGroupingTypes::BaseSetDeltaPointerType m_pMetadataIdsByExpiryHeight;
	};

	/// Delta on top of the metadata cache.
	class MetadataV1CacheDelta : public ReadOnlyViewSupplier<BasicMetadataV1CacheDelta> {
	public:
		/// Creates a delta around \a metadataSets.
		MetadataV1CacheDelta(const MetadataV1CacheTypes::BaseSetDeltaPointers& metadataSets)
				: ReadOnlyViewSupplier(metadataSets)
		{}
	};
}}
