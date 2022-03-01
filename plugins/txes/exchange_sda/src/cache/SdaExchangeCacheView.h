/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaExchangeBaseSets.h"
#include "SdaExchangeCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Mixins used by the SDA-SDA exchange cache view.
	using SdaExchangeCacheViewMixins = PatriciaTreeCacheMixins<SdaExchangeCacheTypes::PrimaryTypes::BaseSetType, SdaExchangeCacheDescriptor>;

	/// Basic view on top of the SDA-SDA exchange cache.
	class BasicSdaExchangeCacheView
			: public utils::MoveOnly
			, public SdaExchangeCacheViewMixins::Size
			, public SdaExchangeCacheViewMixins::Contains
			, public SdaExchangeCacheViewMixins::Iteration
			, public SdaExchangeCacheViewMixins::ConstAccessor
			, public SdaExchangeCacheViewMixins::PatriciaTreeView
			, public SdaExchangeCacheViewMixins::ConfigBasedEnable<config::SdaExchangeConfiguration> {
	public:
		using ReadOnlyView = SdaExchangeCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a sdaExchangeSets.
		explicit BasicSdaExchangeCacheView(
			const SdaExchangeCacheTypes::BaseSets& sdaExchangeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SdaExchangeCacheViewMixins::Size(sdaExchangeSets.Primary)
				, SdaExchangeCacheViewMixins::Contains(sdaExchangeSets.Primary)
				, SdaExchangeCacheViewMixins::Iteration(sdaExchangeSets.Primary)
				, SdaExchangeCacheViewMixins::ConstAccessor(sdaExchangeSets.Primary)
				, SdaExchangeCacheViewMixins::PatriciaTreeView(sdaExchangeSets.PatriciaTree.get())
				, SdaExchangeCacheViewMixins::ConfigBasedEnable<config::SdaExchangeConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the SDA-SDA exchange cache.
	class SdaExchangeCacheView : public ReadOnlyViewSupplier<BasicSdaExchangeCacheView> {
	public:
		/// Creates a view around \a sdaExchangeSets.
		explicit SdaExchangeCacheView(
			const SdaExchangeCacheTypes::BaseSets& sdaExchangeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(sdaExchangeSets, pConfigHolder)
		{}
	};
}}
