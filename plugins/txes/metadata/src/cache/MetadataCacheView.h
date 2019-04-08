/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataCacheSerializers.h"
#include "MetadataCacheTypes.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the metadata cache view.
	struct MetadataCacheViewMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<MetadataCacheTypes::PrimaryTypes::BaseSetType, MetadataCacheDescriptor>;

	public:
		using Size = PrimaryMixins::Size;
		using Iteration = PrimaryMixins::Iteration;
		using ConstAccessor = PrimaryMixins::ConstAccessor;
		using PatriciaTreeView = PrimaryMixins::PatriciaTreeView;
	};

	/// Basic view on top of the metadata cache.
	class BasicMetadataCacheView
			: public utils::MoveOnly
			, public MetadataCacheViewMixins::Size
			, public MetadataCacheViewMixins::Iteration
			, public MetadataCacheViewMixins::ConstAccessor
			, public MetadataCacheViewMixins::PatriciaTreeView {
	public:
		using ReadOnlyView = MetadataCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a metadataSets.
		BasicMetadataCacheView(const MetadataCacheTypes::BaseSets& metadataSets)
				: MetadataCacheViewMixins::Size(metadataSets.Primary)
				, MetadataCacheViewMixins::Iteration(metadataSets.Primary)
				, MetadataCacheViewMixins::ConstAccessor(metadataSets.Primary)
				, MetadataCacheViewMixins::PatriciaTreeView(metadataSets.PatriciaTree.get())
		{}
	};

	/// View on top of the metadata cache.
	class MetadataCacheView : public ReadOnlyViewSupplier<BasicMetadataCacheView> {
	public:
		/// Creates a view around \a metadataSets.
		MetadataCacheView(const MetadataCacheTypes::BaseSets& metadataSets)
				: ReadOnlyViewSupplier(metadataSets)
		{}
	};
}}
