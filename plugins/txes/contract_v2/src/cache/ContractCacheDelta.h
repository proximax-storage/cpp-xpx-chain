/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ContractBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/ContractConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the super contract delta view.
	struct ContractCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<ContractCacheTypes::PrimaryTypes::BaseSetDeltaType, ContractCacheDescriptor>;

	public:
		using Size = PrimaryMixins::Size;
		using Contains = PrimaryMixins::Contains;
		using PatriciaTreeDelta = PrimaryMixins::PatriciaTreeDelta;
		using MutableAccessor = PrimaryMixins::ConstAccessor;
		using ConstAccessor = PrimaryMixins::MutableAccessor;
		using DeltaElements = PrimaryMixins::DeltaElements;
		using BasicInsertRemove = PrimaryMixins::BasicInsertRemove;
		using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::ContractConfiguration>;
	};

	/// Basic delta on top of the super contract cache.
	class BasicContractCacheDelta
			: public utils::MoveOnly
			, public ContractCacheDeltaMixins::Size
			, public ContractCacheDeltaMixins::Contains
			, public ContractCacheDeltaMixins::ConstAccessor
			, public ContractCacheDeltaMixins::MutableAccessor
			, public ContractCacheDeltaMixins::PatriciaTreeDelta
			, public ContractCacheDeltaMixins::BasicInsertRemove
			, public ContractCacheDeltaMixins::DeltaElements
			, public ContractCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = ContractCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a contractSets and \a pConfigHolder.
		explicit BasicContractCacheDelta(
			const ContractCacheTypes::BaseSetDeltaPointers& contractSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ContractCacheDeltaMixins::Size(*contractSets.pPrimary)
				, ContractCacheDeltaMixins::Contains(*contractSets.pPrimary)
				, ContractCacheDeltaMixins::ConstAccessor(*contractSets.pPrimary)
				, ContractCacheDeltaMixins::MutableAccessor(*contractSets.pPrimary)
				, ContractCacheDeltaMixins::PatriciaTreeDelta(*contractSets.pPrimary, contractSets.pPatriciaTree)
				, ContractCacheDeltaMixins::BasicInsertRemove(*contractSets.pPrimary)
				, ContractCacheDeltaMixins::DeltaElements(*contractSets.pPrimary)
				, ContractCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pContractEntries(contractSets.pPrimary)
		{}

	public:
		using ContractCacheDeltaMixins::ConstAccessor::find;
		using ContractCacheDeltaMixins::MutableAccessor::find;

	private:
		ContractCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pContractEntries;
	};

	/// Delta on top of the super contract cache.
	class ContractCacheDelta : public ReadOnlyViewSupplier<BasicContractCacheDelta> {
	public:
		/// Creates a delta around \a contractSets and \a pConfigHolder.
		explicit ContractCacheDelta(
			const ContractCacheTypes::BaseSetDeltaPointers& contractSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(contractSets, pConfigHolder)
		{}
	};
}}
