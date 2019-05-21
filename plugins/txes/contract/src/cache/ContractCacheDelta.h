/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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
