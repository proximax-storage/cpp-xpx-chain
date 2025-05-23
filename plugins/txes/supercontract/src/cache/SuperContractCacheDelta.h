/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/SuperContractConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the super contract delta view.
	struct SuperContractCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<SuperContractCacheTypes::PrimaryTypes::BaseSetDeltaType, SuperContractCacheDescriptor>;

	public:
		using Size = PrimaryMixins::Size;
		using Contains = PrimaryMixins::Contains;
		using PatriciaTreeDelta = PrimaryMixins::PatriciaTreeDelta;
		using MutableAccessor = PrimaryMixins::ConstAccessor;
		using ConstAccessor = PrimaryMixins::MutableAccessor;
		using DeltaElements = PrimaryMixins::DeltaElements;
		using BasicInsertRemove = PrimaryMixins::BasicInsertRemove;
		using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::SuperContractConfiguration>;
	};

	/// Basic delta on top of the super contract cache.
	class BasicSuperContractCacheDelta
			: public utils::MoveOnly
			, public SuperContractCacheDeltaMixins::Size
			, public SuperContractCacheDeltaMixins::Contains
			, public SuperContractCacheDeltaMixins::ConstAccessor
			, public SuperContractCacheDeltaMixins::MutableAccessor
			, public SuperContractCacheDeltaMixins::PatriciaTreeDelta
			, public SuperContractCacheDeltaMixins::BasicInsertRemove
			, public SuperContractCacheDeltaMixins::DeltaElements
			, public SuperContractCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = SuperContractCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a superContractSets and \a pConfigHolder.
		explicit BasicSuperContractCacheDelta(
			const SuperContractCacheTypes::BaseSetDeltaPointers& superContractSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SuperContractCacheDeltaMixins::Size(*superContractSets.pPrimary)
				, SuperContractCacheDeltaMixins::Contains(*superContractSets.pPrimary)
				, SuperContractCacheDeltaMixins::ConstAccessor(*superContractSets.pPrimary)
				, SuperContractCacheDeltaMixins::MutableAccessor(*superContractSets.pPrimary)
				, SuperContractCacheDeltaMixins::PatriciaTreeDelta(*superContractSets.pPrimary, superContractSets.pPatriciaTree)
				, SuperContractCacheDeltaMixins::BasicInsertRemove(*superContractSets.pPrimary)
				, SuperContractCacheDeltaMixins::DeltaElements(*superContractSets.pPrimary)
				, SuperContractCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pSuperContractEntries(superContractSets.pPrimary)
		{}

	public:
		using SuperContractCacheDeltaMixins::ConstAccessor::find;
		using SuperContractCacheDeltaMixins::MutableAccessor::find;

	private:
		SuperContractCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pSuperContractEntries;
	};

	/// Delta on top of the super contract cache.
	class SuperContractCacheDelta : public ReadOnlyViewSupplier<BasicSuperContractCacheDelta> {
	public:
		/// Creates a delta around \a superContractSets and \a pConfigHolder.
		explicit SuperContractCacheDelta(
			const SuperContractCacheTypes::BaseSetDeltaPointers& superContractSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(superContractSets, pConfigHolder)
		{}
	};
}}
