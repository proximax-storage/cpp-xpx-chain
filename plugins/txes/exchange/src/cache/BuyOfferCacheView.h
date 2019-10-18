/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BuyOfferBaseSets.h"
#include "BuyOfferCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the buy offer cache view.
	using BuyOfferCacheViewMixins = PatriciaTreeCacheMixins<BuyOfferCacheTypes::PrimaryTypes::BaseSetType, BuyOfferCacheDescriptor>;

	/// Basic view on top of the buy offer cache.
	class BasicBuyOfferCacheView
			: public utils::MoveOnly
			, public BuyOfferCacheViewMixins::Size
			, public BuyOfferCacheViewMixins::Contains
			, public BuyOfferCacheViewMixins::Iteration
			, public BuyOfferCacheViewMixins::ConstAccessor
			, public BuyOfferCacheViewMixins::PatriciaTreeView
			, public BuyOfferCacheViewMixins::Enable
			, public BuyOfferCacheViewMixins::Height {
	public:
		using ReadOnlyView = BuyOfferCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a buyOfferSets.
		explicit BasicBuyOfferCacheView(const BuyOfferCacheTypes::BaseSets& buyOfferSets)
				: BuyOfferCacheViewMixins::Size(buyOfferSets.Primary)
				, BuyOfferCacheViewMixins::Contains(buyOfferSets.Primary)
				, BuyOfferCacheViewMixins::Iteration(buyOfferSets.Primary)
				, BuyOfferCacheViewMixins::ConstAccessor(buyOfferSets.Primary)
				, BuyOfferCacheViewMixins::PatriciaTreeView(buyOfferSets.PatriciaTree.get())
		{}
	};

	/// View on top of the buy offer cache.
	class BuyOfferCacheView : public ReadOnlyViewSupplier<BasicBuyOfferCacheView> {
	public:
		/// Creates a view around \a buyOfferSets.
		explicit BuyOfferCacheView(const BuyOfferCacheTypes::BaseSets& buyOfferSets)
				: ReadOnlyViewSupplier(buyOfferSets)
		{}
	};
}}
