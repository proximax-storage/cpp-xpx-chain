/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataV1CacheSerializers.h"
#include "MetadataV1CacheTypes.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the metadata cache view.
	struct MetadataV1CacheViewMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<MetadataV1CacheTypes::PrimaryTypes::BaseSetType, MetadataV1CacheDescriptor>;

	public:
		using Size = PrimaryMixins::Size;
		using Contains = PrimaryMixins::Contains;
		using Iteration = PrimaryMixins::Iteration;
		using ConstAccessor = PrimaryMixins::ConstAccessor;
		using PatriciaTreeView = PrimaryMixins::PatriciaTreeView;
		using Enable = PrimaryMixins::Enable;
		using Height = PrimaryMixins::Height;
	};

	/// Basic view on top of the metadata cache.
	class BasicMetadataV1CacheView
			: public utils::MoveOnly
			, public MetadataV1CacheViewMixins::Size
			, public MetadataV1CacheViewMixins::Contains
			, public MetadataV1CacheViewMixins::Iteration
			, public MetadataV1CacheViewMixins::ConstAccessor
			, public MetadataV1CacheViewMixins::PatriciaTreeView
			, public MetadataV1CacheViewMixins::Enable
			, public MetadataV1CacheViewMixins::Height {
	public:
		using ReadOnlyView = MetadataV1CacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a metadataSets.
		BasicMetadataV1CacheView(const MetadataV1CacheTypes::BaseSets& metadataSets)
				: MetadataV1CacheViewMixins::Size(metadataSets.Primary)
				, MetadataV1CacheViewMixins::Contains(metadataSets.Primary)
				, MetadataV1CacheViewMixins::Iteration(metadataSets.Primary)
				, MetadataV1CacheViewMixins::ConstAccessor(metadataSets.Primary)
				, MetadataV1CacheViewMixins::PatriciaTreeView(metadataSets.PatriciaTree.get())
		{}
	};

	/// View on top of the metadata cache.
	class MetadataV1CacheView : public ReadOnlyViewSupplier<BasicMetadataV1CacheView> {
	public:
		/// Creates a view around \a metadataSets.
		MetadataV1CacheView(const MetadataV1CacheTypes::BaseSets& metadataSets)
				: ReadOnlyViewSupplier(metadataSets)
		{}
	};
}}
