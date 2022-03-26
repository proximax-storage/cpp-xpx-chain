/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LPBaseSets.h"
#include "LPCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/LiquidityProviderConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive cache view.
	using LPCacheViewMixins = PatriciaTreeCacheMixins<LPCacheTypes::PrimaryTypes::BaseSetType, LPCacheDescriptor>;

	/// Basic view on top of the drive cache.
	class BasicLPCacheView
			: public utils::MoveOnly
			, public LPCacheViewMixins::Size
			, public LPCacheViewMixins::Contains
			, public LPCacheViewMixins::Iteration
			, public LPCacheViewMixins::ConstAccessor
			, public LPCacheViewMixins::PatriciaTreeView
			, public LPCacheViewMixins::ConfigBasedEnable<config::LiquidityProviderConfiguration> {
	public:
		using ReadOnlyView = LPCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit BasicLPCacheView(const LPCacheTypes::BaseSets& driveSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: LPCacheViewMixins::Size(driveSets.Primary)
				, LPCacheViewMixins::Contains(driveSets.Primary)
				, LPCacheViewMixins::Iteration(driveSets.Primary)
				, LPCacheViewMixins::ConstAccessor(driveSets.Primary)
				, LPCacheViewMixins::PatriciaTreeView(driveSets.PatriciaTree.get())
				, LPCacheViewMixins::ConfigBasedEnable<config::LiquidityProviderConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the drive cache.
	class LPCacheView : public ReadOnlyViewSupplier<BasicLPCacheView> {
	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit LPCacheView(const LPCacheTypes::BaseSets& driveSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}
