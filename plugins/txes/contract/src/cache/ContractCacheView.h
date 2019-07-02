/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
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
			, public ContractCacheViewMixins::PatriciaTreeView
			, public ContractCacheViewMixins::Enable {
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
