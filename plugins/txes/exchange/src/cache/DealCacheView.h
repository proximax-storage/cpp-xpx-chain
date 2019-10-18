/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DealBaseSets.h"
#include "DealCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the deal cache view.
	using DealCacheViewMixins = PatriciaTreeCacheMixins<DealCacheTypes::PrimaryTypes::BaseSetType, DealCacheDescriptor>;

	/// Basic view on top of the deal cache.
	class BasicDealCacheView
			: public utils::MoveOnly
			, public DealCacheViewMixins::Size
			, public DealCacheViewMixins::Contains
			, public DealCacheViewMixins::Iteration
			, public DealCacheViewMixins::ConstAccessor
			, public DealCacheViewMixins::PatriciaTreeView
			, public DealCacheViewMixins::Enable
			, public DealCacheViewMixins::Height {
	public:
		using ReadOnlyView = DealCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a dealSets.
		explicit BasicDealCacheView(const DealCacheTypes::BaseSets& dealSets)
				: DealCacheViewMixins::Size(dealSets.Primary)
				, DealCacheViewMixins::Contains(dealSets.Primary)
				, DealCacheViewMixins::Iteration(dealSets.Primary)
				, DealCacheViewMixins::ConstAccessor(dealSets.Primary)
				, DealCacheViewMixins::PatriciaTreeView(dealSets.PatriciaTree.get())
		{}
	};

	/// View on top of the deal cache.
	class DealCacheView : public ReadOnlyViewSupplier<BasicDealCacheView> {
	public:
		/// Creates a view around \a dealSets.
		explicit DealCacheView(const DealCacheTypes::BaseSets& dealSets)
				: ReadOnlyViewSupplier(dealSets)
		{}
	};
}}
