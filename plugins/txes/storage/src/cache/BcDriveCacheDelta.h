/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/config/StorageConfiguration.h"
#include "BcDriveBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive delta view.
	struct BcDriveCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<BcDriveCacheTypes::PrimaryTypes::BaseSetDeltaType, BcDriveCacheDescriptor>;

	public:
		using Size = PrimaryMixins::Size;
		using Contains = PrimaryMixins::Contains;
		using PatriciaTreeDelta = PrimaryMixins::PatriciaTreeDelta;
		using MutableAccessor = PrimaryMixins::ConstAccessor;
		using ConstAccessor = PrimaryMixins::MutableAccessor;
		using DeltaElements = PrimaryMixins::DeltaElements;
		using BasicInsertRemove = PrimaryMixins::BasicInsertRemove;
		using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::StorageConfiguration>;
	};

	/// Basic delta on top of the drive cache.
	class BasicBcDriveCacheDelta
			: public utils::MoveOnly
			, public BcDriveCacheDeltaMixins::Size
			, public BcDriveCacheDeltaMixins::Contains
			, public BcDriveCacheDeltaMixins::ConstAccessor
			, public BcDriveCacheDeltaMixins::MutableAccessor
			, public BcDriveCacheDeltaMixins::PatriciaTreeDelta
			, public BcDriveCacheDeltaMixins::BasicInsertRemove
			, public BcDriveCacheDeltaMixins::DeltaElements
			, public BcDriveCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = BcDriveCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a driveSets and \a pConfigHolder.
		explicit BasicBcDriveCacheDelta(
			const BcDriveCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BcDriveCacheDeltaMixins::Size(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::Contains(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::ConstAccessor(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::MutableAccessor(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::PatriciaTreeDelta(*driveSets.pPrimary, driveSets.pPatriciaTree)
				, BcDriveCacheDeltaMixins::BasicInsertRemove(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::DeltaElements(*driveSets.pPrimary)
				, BcDriveCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pDriveEntries(driveSets.pPrimary)
		{}

	public:
		using BcDriveCacheDeltaMixins::ConstAccessor::find;
		using BcDriveCacheDeltaMixins::MutableAccessor::find;

	private:
		BcDriveCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pDriveEntries;
	};

	/// Delta on top of the drive cache.
	class BcDriveCacheDelta : public ReadOnlyViewSupplier<BasicBcDriveCacheDelta> {
	public:
		/// Creates a delta around \a driveSets and \a pConfigHolder.
		explicit BcDriveCacheDelta(
			const BcDriveCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}
