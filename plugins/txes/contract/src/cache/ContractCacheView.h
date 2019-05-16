/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ContractBaseSets.h"
#include "ContractCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the contract cache view.
	using ContractCacheViewMixins = PatriciaTreeCacheMixins<ContractCacheTypes::PrimaryTypes::BaseSetType, ContractCacheDescriptor>;

	/// Basic view on top of the contract cache.
	class BasicContractCacheView
			: public utils::MoveOnly
			, public ContractCacheViewMixins::Size
			, public ContractCacheViewMixins::Contains
			, public ContractCacheViewMixins::Iteration
			, public ContractCacheViewMixins::ConstAccessor
			, public ContractCacheViewMixins::PatriciaTreeView {
	public:
		using ReadOnlyView = ContractCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a contractSets.
		explicit BasicContractCacheView(const ContractCacheTypes::BaseSets& contractSets)
				: ContractCacheViewMixins::Size(contractSets.Primary)
				, ContractCacheViewMixins::Contains(contractSets.Primary)
				, ContractCacheViewMixins::Iteration(contractSets.Primary)
				, ContractCacheViewMixins::ConstAccessor(contractSets.Primary)
				, ContractCacheViewMixins::PatriciaTreeView(contractSets.PatriciaTree.get())
		{}
	};

	/// View on top of the contract cache.
	class ContractCacheView : public ReadOnlyViewSupplier<BasicContractCacheView> {
	public:
		/// Creates a view around \a contractSets.
		explicit ContractCacheView(const ContractCacheTypes::BaseSets& contractSets)
				: ReadOnlyViewSupplier(contractSets)
		{}
	};
}}
