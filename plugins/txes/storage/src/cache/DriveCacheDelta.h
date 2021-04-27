/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/config/StorageConfiguration.h"
#include "DriveBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive delta view.
	struct DriveCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<DriveCacheTypes::PrimaryTypes::BaseSetDeltaType, DriveCacheDescriptor>;

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
	class BasicDriveCacheDelta
			: public utils::MoveOnly
			, public DriveCacheDeltaMixins::Size
			, public DriveCacheDeltaMixins::Contains
			, public DriveCacheDeltaMixins::ConstAccessor
			, public DriveCacheDeltaMixins::MutableAccessor
			, public DriveCacheDeltaMixins::PatriciaTreeDelta
			, public DriveCacheDeltaMixins::BasicInsertRemove
			, public DriveCacheDeltaMixins::DeltaElements
			, public DriveCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = DriveCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a driveSets and \a pConfigHolder.
		explicit BasicDriveCacheDelta(
			const DriveCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: DriveCacheDeltaMixins::Size(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::Contains(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::ConstAccessor(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::MutableAccessor(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::PatriciaTreeDelta(*driveSets.pPrimary, driveSets.pPatriciaTree)
				, DriveCacheDeltaMixins::BasicInsertRemove(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::DeltaElements(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pDriveEntries(driveSets.pPrimary)
		{}

	public:
		using DriveCacheDeltaMixins::ConstAccessor::find;
		using DriveCacheDeltaMixins::MutableAccessor::find;

	private:
		DriveCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pDriveEntries;
	};

	/// Delta on top of the drive cache.
	class DriveCacheDelta : public ReadOnlyViewSupplier<BasicDriveCacheDelta> {
	public:
		/// Creates a delta around \a driveSets and \a pConfigHolder.
		explicit DriveCacheDelta(
			const DriveCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}
