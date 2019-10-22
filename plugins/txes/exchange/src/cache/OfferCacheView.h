/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferBaseSets.h"
#include "OfferCacheSerializers.h"
#include "ExchangeCacheTools.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Mixins used by the offer cache view.
	using OfferCacheViewMixins = PatriciaTreeCacheMixins<OfferCacheTypes::PrimaryTypes::BaseSetType, OfferCacheDescriptor>;

	/// Basic view on top of the offer cache.
	class BasicOfferCacheView
			: public utils::MoveOnly
			, public OfferCacheViewMixins::Size
			, public OfferCacheViewMixins::Contains
			, public OfferCacheViewMixins::Iteration
			, public OfferCacheViewMixins::ConstAccessor
			, public OfferCacheViewMixins::PatriciaTreeView
			, public OfferCacheViewMixins::Enable
			, public OfferCacheViewMixins::Height {
	public:
		using ReadOnlyView = OfferCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a offerSets.
		explicit BasicOfferCacheView(
			const OfferCacheTypes::BaseSets& offerSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: OfferCacheViewMixins::Size(offerSets.Primary)
				, OfferCacheViewMixins::Contains(offerSets.Primary)
				, OfferCacheViewMixins::Iteration(offerSets.Primary)
				, OfferCacheViewMixins::ConstAccessor(offerSets.Primary)
				, OfferCacheViewMixins::PatriciaTreeView(offerSets.PatriciaTree.get())
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		bool enabled() const {
			return ExchangePluginEnabled(m_pConfigHolder, height());
		}

	private:
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// View on top of the offer cache.
	class OfferCacheView : public ReadOnlyViewSupplier<BasicOfferCacheView> {
	public:
		/// Creates a view around \a offerSets.
		explicit OfferCacheView(
			const OfferCacheTypes::BaseSets& offerSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(offerSets, pConfigHolder)
		{}
	};
}}
