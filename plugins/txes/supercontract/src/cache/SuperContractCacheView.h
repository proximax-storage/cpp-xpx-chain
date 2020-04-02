/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractBaseSets.h"
#include "SuperContractCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Mixins used by the super contract cache view.
	using SuperContractCacheViewMixins = PatriciaTreeCacheMixins<SuperContractCacheTypes::PrimaryTypes::BaseSetType, SuperContractCacheDescriptor>;

	/// Basic view on top of the super contract cache.
	class BasicSuperContractCacheView
			: public utils::MoveOnly
			, public SuperContractCacheViewMixins::Size
			, public SuperContractCacheViewMixins::Contains
			, public SuperContractCacheViewMixins::Iteration
			, public SuperContractCacheViewMixins::ConstAccessor
			, public SuperContractCacheViewMixins::PatriciaTreeView
			, public SuperContractCacheViewMixins::ConfigBasedEnable<config::SuperContractConfiguration> {
	public:
		using ReadOnlyView = SuperContractCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a superContractSets and \a pConfigHolder.
		explicit BasicSuperContractCacheView(const SuperContractCacheTypes::BaseSets& superContractSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SuperContractCacheViewMixins::Size(superContractSets.Primary)
				, SuperContractCacheViewMixins::Contains(superContractSets.Primary)
				, SuperContractCacheViewMixins::Iteration(superContractSets.Primary)
				, SuperContractCacheViewMixins::ConstAccessor(superContractSets.Primary)
				, SuperContractCacheViewMixins::PatriciaTreeView(superContractSets.PatriciaTree.get())
				, SuperContractCacheViewMixins::ConfigBasedEnable<config::SuperContractConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the super contract cache.
	class SuperContractCacheView : public ReadOnlyViewSupplier<BasicSuperContractCacheView> {
	public:
		/// Creates a view around \a superContractSets and \a pConfigHolder.
		explicit SuperContractCacheView(const SuperContractCacheTypes::BaseSets& superContractSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(superContractSets, pConfigHolder)
		{}
	};
}}
