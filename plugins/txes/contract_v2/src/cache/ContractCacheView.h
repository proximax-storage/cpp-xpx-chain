/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ContractBaseSets.h"
#include "ContractCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Mixins used by the super contract cache view.
	using ContractCacheViewMixins = PatriciaTreeCacheMixins<ContractCacheTypes::PrimaryTypes::BaseSetType, ContractCacheDescriptor>;

	/// Basic view on top of the super contract cache.
	class BasicContractCacheView
			: public utils::MoveOnly
			, public ContractCacheViewMixins::Size
			, public ContractCacheViewMixins::Contains
			, public ContractCacheViewMixins::Iteration
			, public ContractCacheViewMixins::ConstAccessor
			, public ContractCacheViewMixins::PatriciaTreeView
			, public ContractCacheViewMixins::ConfigBasedEnable<config::ContractConfiguration> {
	public:
		using ReadOnlyView = ContractCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a contractSets and \a pConfigHolder.
		explicit BasicContractCacheView(const ContractCacheTypes::BaseSets& contractSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ContractCacheViewMixins::Size(contractSets.Primary)
				, ContractCacheViewMixins::Contains(contractSets.Primary)
				, ContractCacheViewMixins::Iteration(contractSets.Primary)
				, ContractCacheViewMixins::ConstAccessor(contractSets.Primary)
				, ContractCacheViewMixins::PatriciaTreeView(contractSets.PatriciaTree.get())
				, ContractCacheViewMixins::ConfigBasedEnable<config::ContractConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the super contract cache.
	class ContractCacheView : public ReadOnlyViewSupplier<BasicContractCacheView> {
	public:
		/// Creates a view around \a contractSets and \a pConfigHolder.
		explicit ContractCacheView(const ContractCacheTypes::BaseSets& contractSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(contractSets, pConfigHolder)
		{}
	};
}}
