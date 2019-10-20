/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferDeadlineBaseSets.h"
#include "OfferDeadlineCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the offer deadline cache view.
	using OfferDeadlineCacheViewMixins = PatriciaTreeCacheMixins<OfferDeadlineCacheTypes::PrimaryTypes::BaseSetType, OfferDeadlineCacheDescriptor>;

	/// Basic view on top of the offer deadline cache.
	class BasicOfferDeadlineCacheView
			: public utils::MoveOnly
			, public OfferDeadlineCacheViewMixins::Size
			, public OfferDeadlineCacheViewMixins::Contains
			, public OfferDeadlineCacheViewMixins::Iteration
			, public OfferDeadlineCacheViewMixins::ConstAccessor
			, public OfferDeadlineCacheViewMixins::PatriciaTreeView
			, public OfferDeadlineCacheViewMixins::Enable
			, public OfferDeadlineCacheViewMixins::Height {
	public:
		using ReadOnlyView = OfferDeadlineCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a offerDeadlineSets.
		explicit BasicOfferDeadlineCacheView(const OfferDeadlineCacheTypes::BaseSets& offerDeadlineSets)
				: OfferDeadlineCacheViewMixins::Size(offerDeadlineSets.Primary)
				, OfferDeadlineCacheViewMixins::Contains(offerDeadlineSets.Primary)
				, OfferDeadlineCacheViewMixins::Iteration(offerDeadlineSets.Primary)
				, OfferDeadlineCacheViewMixins::ConstAccessor(offerDeadlineSets.Primary)
				, OfferDeadlineCacheViewMixins::PatriciaTreeView(offerDeadlineSets.PatriciaTree.get())
		{}
	};

	/// View on top of the offer deadline cache.
	class OfferDeadlineCacheView : public ReadOnlyViewSupplier<BasicOfferDeadlineCacheView> {
	public:
		/// Creates a view around \a offerDeadlineSets.
		explicit OfferDeadlineCacheView(const OfferDeadlineCacheTypes::BaseSets& offerDeadlineSets)
				: ReadOnlyViewSupplier(offerDeadlineSets)
		{}
	};
}}
