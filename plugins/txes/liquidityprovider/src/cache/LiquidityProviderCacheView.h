/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LiquidityProviderBaseSets.h"
#include "LiquidityProviderCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/LiquidityProviderConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the LiquidityProvider cache view.
	using LiquidityProviderCacheViewMixins = PatriciaTreeCacheMixins<LiquidityProviderCacheTypes::PrimaryTypes::BaseSetType, LiquidityProviderCacheDescriptor>;

	/// Basic view on top of the LiquidityProvider cache.
	class BasicLiquidityProviderCacheView
			: public utils::MoveOnly
			, public LiquidityProviderCacheViewMixins::Size
			, public LiquidityProviderCacheViewMixins::Contains
			, public LiquidityProviderCacheViewMixins::Iteration
			, public LiquidityProviderCacheViewMixins::ConstAccessor
			, public LiquidityProviderCacheViewMixins::PatriciaTreeView
			, public LiquidityProviderCacheViewMixins::ConfigBasedEnable<config::LiquidityProviderConfiguration> {
	public:
		using ReadOnlyView = LiquidityProviderCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a LiquidityProviderSets and \a pConfigHolder.
		explicit BasicLiquidityProviderCacheView(const LiquidityProviderCacheTypes::BaseSets& LiquidityProviderSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: LiquidityProviderCacheViewMixins::Size(LiquidityProviderSets.Primary)
				, LiquidityProviderCacheViewMixins::Contains(LiquidityProviderSets.Primary)
				, LiquidityProviderCacheViewMixins::Iteration(LiquidityProviderSets.Primary)
				, LiquidityProviderCacheViewMixins::ConstAccessor(LiquidityProviderSets.Primary)
				, LiquidityProviderCacheViewMixins::PatriciaTreeView(LiquidityProviderSets.PatriciaTree.get())
				, LiquidityProviderCacheViewMixins::ConfigBasedEnable<config::LiquidityProviderConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the LiquidityProvider cache.
	class LiquidityProviderCacheView : public ReadOnlyViewSupplier<BasicLiquidityProviderCacheView> {
	public:
		/// Creates a view around \a LiquidityProviderSets and \a pConfigHolder.
		explicit LiquidityProviderCacheView(const LiquidityProviderCacheTypes::BaseSets& LiquidityProviderSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(LiquidityProviderSets, pConfigHolder)
		{}
	};
}}
