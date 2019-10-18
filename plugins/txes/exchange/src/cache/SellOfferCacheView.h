/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SellOfferBaseSets.h"
#include "SellOfferCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the sell offer cache view.
	using SellOfferCacheViewMixins = PatriciaTreeCacheMixins<SellOfferCacheTypes::PrimaryTypes::BaseSetType, SellOfferCacheDescriptor>;

	/// Basic view on top of the sell offer cache.
	class BasicSellOfferCacheView
			: public utils::MoveOnly
			, public SellOfferCacheViewMixins::Size
			, public SellOfferCacheViewMixins::Contains
			, public SellOfferCacheViewMixins::Iteration
			, public SellOfferCacheViewMixins::ConstAccessor
			, public SellOfferCacheViewMixins::PatriciaTreeView
			, public SellOfferCacheViewMixins::Enable
			, public SellOfferCacheViewMixins::Height {
	public:
		using ReadOnlyView = SellOfferCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a sellOfferSets.
		explicit BasicSellOfferCacheView(const SellOfferCacheTypes::BaseSets& sellOfferSets)
				: SellOfferCacheViewMixins::Size(sellOfferSets.Primary)
				, SellOfferCacheViewMixins::Contains(sellOfferSets.Primary)
				, SellOfferCacheViewMixins::Iteration(sellOfferSets.Primary)
				, SellOfferCacheViewMixins::ConstAccessor(sellOfferSets.Primary)
				, SellOfferCacheViewMixins::PatriciaTreeView(sellOfferSets.PatriciaTree.get())
		{}
	};

	/// View on top of the sell offer cache.
	class SellOfferCacheView : public ReadOnlyViewSupplier<BasicSellOfferCacheView> {
	public:
		/// Creates a view around \a sellOfferSets.
		explicit SellOfferCacheView(const SellOfferCacheTypes::BaseSets& sellOfferSets)
				: ReadOnlyViewSupplier(sellOfferSets)
		{}
	};
}}
