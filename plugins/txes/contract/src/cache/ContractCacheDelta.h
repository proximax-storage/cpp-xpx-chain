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
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the contract cache delta.
	using ContractCacheDeltaMixins = PatriciaTreeCacheMixins<ContractCacheTypes::PrimaryTypes::BaseSetDeltaType, ContractCacheDescriptor>;

	/// Basic delta on top of the contract cache.
	class BasicContractCacheDelta
			: public utils::MoveOnly
			, public ContractCacheDeltaMixins::Size
			, public ContractCacheDeltaMixins::Contains
			, public ContractCacheDeltaMixins::ConstAccessor
			, public ContractCacheDeltaMixins::MutableAccessor
			, public ContractCacheDeltaMixins::PatriciaTreeDelta
			, public ContractCacheDeltaMixins::BasicInsertRemove
			, public ContractCacheDeltaMixins::DeltaElements {
	public:
		using ReadOnlyView = ContractCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a contractSets.
		explicit BasicContractCacheDelta(const ContractCacheTypes::BaseSetDeltaPointers& contractSets)
				: ContractCacheDeltaMixins::Size(*contractSets.pPrimary)
				, ContractCacheDeltaMixins::Contains(*contractSets.pPrimary)
				, ContractCacheDeltaMixins::ConstAccessor(*contractSets.pPrimary)
				, ContractCacheDeltaMixins::MutableAccessor(*contractSets.pPrimary)
				, ContractCacheDeltaMixins::PatriciaTreeDelta(*contractSets.pPrimary, contractSets.pPatriciaTree)
				, ContractCacheDeltaMixins::BasicInsertRemove(*contractSets.pPrimary)
				, ContractCacheDeltaMixins::DeltaElements(*contractSets.pPrimary)
				, m_pContractEntries(contractSets.pPrimary)
		{}

	public:
		using ContractCacheDeltaMixins::ConstAccessor::find;
		using ContractCacheDeltaMixins::MutableAccessor::find;

	private:
		ContractCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pContractEntries;
	};

	/// Delta on top of the contract cache.
	class ContractCacheDelta : public ReadOnlyViewSupplier<BasicContractCacheDelta> {
	public:
		/// Creates a delta around \a contractSets.
		explicit ContractCacheDelta(const ContractCacheTypes::BaseSetDeltaPointers& contractSets)
				: ReadOnlyViewSupplier(contractSets)
		{}
	};
}}
