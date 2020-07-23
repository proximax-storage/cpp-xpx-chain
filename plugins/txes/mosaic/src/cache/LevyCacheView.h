/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include "LevyBaseSets.h"
#include "LevyCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {
		
	/// Mixins used by the catapult upgrade cache view.
	using LevyCacheViewMixins = PatriciaTreeCacheMixins<LevyCacheTypes::PrimaryTypes::BaseSetType, LevyCacheDescriptor>;
		
	/// Basic view on top of the catapult upgrade cache.
	class BasicLevyCacheView
		: public utils::MoveOnly
			, public LevyCacheViewMixins::Size
			, public LevyCacheViewMixins::Contains
			, public LevyCacheViewMixins::Iteration
			, public LevyCacheViewMixins::ConstAccessor
			, public LevyCacheViewMixins::PatriciaTreeView
			, public LevyCacheViewMixins::ConfigBasedEnable<config::MosaicConfiguration> {
	public:
		using ReadOnlyView = LevyCacheTypes::CacheReadOnlyType;
		
	public:
		/// Creates a view around \a LevySets.
		explicit BasicLevyCacheView(const LevyCacheTypes::BaseSets& LevySets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: LevyCacheViewMixins::Size(LevySets.Primary)
			, LevyCacheViewMixins::Contains(LevySets.Primary)
			, LevyCacheViewMixins::Iteration(LevySets.Primary)
			, LevyCacheViewMixins::ConstAccessor(LevySets.Primary)
			, LevyCacheViewMixins::PatriciaTreeView(LevySets.PatriciaTree.get())
			, LevyCacheViewMixins::ConfigBasedEnable<config::MosaicConfiguration>(pConfigHolder, [](const auto& config) { return config.LevyEnabled; })
		{}
	};
		
	/// View on top of the catapult upgrade cache.
	class LevyCacheView : public ReadOnlyViewSupplier<BasicLevyCacheView> {
	public:
		/// Creates a view around \a LevySets.
		explicit LevyCacheView(const LevyCacheTypes::BaseSets& LevySets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: ReadOnlyViewSupplier(LevySets, pConfigHolder)
			{}
	};
}}
