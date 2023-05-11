/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveContractBaseSets.h"
#include "DriveContractCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Mixins used by the drivecontract cache view.
	using DriveContractCacheViewMixins = PatriciaTreeCacheMixins<DriveContractCacheTypes::PrimaryTypes::BaseSetType, DriveContractCacheDescriptor>;

	/// Basic view on top of the drivecontract cache.
	class BasicDriveContractCacheView
			: public utils::MoveOnly
			, public DriveContractCacheViewMixins::Size
			, public DriveContractCacheViewMixins::Contains
			, public DriveContractCacheViewMixins::Iteration
			, public DriveContractCacheViewMixins::ConstAccessor
			, public DriveContractCacheViewMixins::PatriciaTreeView
			, public DriveContractCacheViewMixins::ConfigBasedEnable<config::SuperContractV2Configuration> {
	public:
		using ReadOnlyView = DriveContractCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a driveContractSets and \a pConfigHolder.
		explicit BasicDriveContractCacheView(const DriveContractCacheTypes::BaseSets& driveContractSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: DriveContractCacheViewMixins::Size(driveContractSets.Primary)
				, DriveContractCacheViewMixins::Contains(driveContractSets.Primary)
				, DriveContractCacheViewMixins::Iteration(driveContractSets.Primary)
				, DriveContractCacheViewMixins::ConstAccessor(driveContractSets.Primary)
				, DriveContractCacheViewMixins::PatriciaTreeView(driveContractSets.PatriciaTree.get())
				, DriveContractCacheViewMixins::ConfigBasedEnable<config::SuperContractV2Configuration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the drivecontract cache.
	class DriveContractCacheView : public ReadOnlyViewSupplier<BasicDriveContractCacheView> {
	public:
		/// Creates a view around \a driveContractSets and \a pConfigHolder.
		explicit DriveContractCacheView(const DriveContractCacheTypes::BaseSets& driveContractSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveContractSets, pConfigHolder)
		{}
	};
}}
