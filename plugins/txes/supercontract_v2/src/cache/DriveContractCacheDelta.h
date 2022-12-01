/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveContractBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/SuperContractConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the drivecontract delta view.
	struct DriveContractCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<DriveContractCacheTypes::PrimaryTypes::BaseSetDeltaType, DriveContractCacheDescriptor>;

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

	/// Basic delta on top of the drivecontract cache.
	class BasicDriveContractCacheDelta
			: public utils::MoveOnly
			, public DriveContractCacheDeltaMixins::Size
			, public DriveContractCacheDeltaMixins::Contains
			, public DriveContractCacheDeltaMixins::ConstAccessor
			, public DriveContractCacheDeltaMixins::MutableAccessor
			, public DriveContractCacheDeltaMixins::PatriciaTreeDelta
			, public DriveContractCacheDeltaMixins::BasicInsertRemove
			, public DriveContractCacheDeltaMixins::DeltaElements
			, public DriveContractCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = DriveContractCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a driveContractSets and \a pConfigHolder.
		explicit BasicDriveContractCacheDelta(
			const DriveContractCacheTypes::BaseSetDeltaPointers& driveContractSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: DriveContractCacheDeltaMixins::Size(*driveContractSets.pPrimary)
				, DriveContractCacheDeltaMixins::Contains(*driveContractSets.pPrimary)
				, DriveContractCacheDeltaMixins::ConstAccessor(*driveContractSets.pPrimary)
				, DriveContractCacheDeltaMixins::MutableAccessor(*driveContractSets.pPrimary)
				, DriveContractCacheDeltaMixins::PatriciaTreeDelta(*driveContractSets.pPrimary, driveContractSets.pPatriciaTree)
				, DriveContractCacheDeltaMixins::BasicInsertRemove(*driveContractSets.pPrimary)
				, DriveContractCacheDeltaMixins::DeltaElements(*driveContractSets.pPrimary)
				, DriveContractCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pDriveContractEntries(driveContractSets.pPrimary)
		{}

	public:
		using DriveContractCacheDeltaMixins::ConstAccessor::find;
		using DriveContractCacheDeltaMixins::MutableAccessor::find;

	private:
		DriveContractCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pDriveContractEntries;
	};

	/// Delta on top of the drivecontract cache.
	class DriveContractCacheDelta : public ReadOnlyViewSupplier<BasicDriveContractCacheDelta> {
	public:
		/// Creates a delta around \a driveContractSets and \a pConfigHolder.
		explicit DriveContractCacheDelta(
			const DriveContractCacheTypes::BaseSetDeltaPointers& driveContractSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveContractSets, pConfigHolder)
		{}
	};
}}
