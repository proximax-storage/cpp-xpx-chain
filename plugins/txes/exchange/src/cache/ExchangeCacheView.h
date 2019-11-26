/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeBaseSets.h"
#include "ExchangeCacheSerializers.h"
#include "ExchangeCacheTools.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Mixins used by the exchange cache view.
	using ExchangeCacheViewMixins = PatriciaTreeCacheMixins<ExchangeCacheTypes::PrimaryTypes::BaseSetType, ExchangeCacheDescriptor>;

	/// Basic view on top of the exchange cache.
	class BasicExchangeCacheView
			: public utils::MoveOnly
			, public ExchangeCacheViewMixins::Size
			, public ExchangeCacheViewMixins::Contains
			, public ExchangeCacheViewMixins::Iteration
			, public ExchangeCacheViewMixins::ConstAccessor
			, public ExchangeCacheViewMixins::PatriciaTreeView
			, public ExchangeCacheViewMixins::Enable
			, public ExchangeCacheViewMixins::Height {
	public:
		using ReadOnlyView = ExchangeCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a exchangeSets.
		explicit BasicExchangeCacheView(
			const ExchangeCacheTypes::BaseSets& exchangeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ExchangeCacheViewMixins::Size(exchangeSets.Primary)
				, ExchangeCacheViewMixins::Contains(exchangeSets.Primary)
				, ExchangeCacheViewMixins::Iteration(exchangeSets.Primary)
				, ExchangeCacheViewMixins::ConstAccessor(exchangeSets.Primary)
				, ExchangeCacheViewMixins::PatriciaTreeView(exchangeSets.PatriciaTree.get())
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		bool enabled() const {
			return ExchangePluginEnabled(m_pConfigHolder, height());
		}

	private:
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// View on top of the exchange cache.
	class ExchangeCacheView : public ReadOnlyViewSupplier<BasicExchangeCacheView> {
	public:
		/// Creates a view around \a exchangeSets.
		explicit ExchangeCacheView(
			const ExchangeCacheTypes::BaseSets& exchangeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(exchangeSets, pConfigHolder)
		{}
	};
}}
