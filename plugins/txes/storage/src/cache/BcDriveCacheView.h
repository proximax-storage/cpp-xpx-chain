/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BcDriveBaseSets.h"
#include "BcDriveCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive cache view.
	using BcDriveCacheViewMixins = PatriciaTreeCacheMixins<BcDriveCacheTypes::PrimaryTypes::BaseSetType, BcDriveCacheDescriptor>;

	/// Basic view on top of the drive cache.
	class BasicBcDriveCacheView
			: public utils::MoveOnly
			, public BcDriveCacheViewMixins::Size
			, public BcDriveCacheViewMixins::Contains
			, public BcDriveCacheViewMixins::Iteration
			, public BcDriveCacheViewMixins::ConstAccessor
			, public BcDriveCacheViewMixins::PatriciaTreeView
			, public BcDriveCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration> {
	public:
		using ReadOnlyView = BcDriveCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit BasicBcDriveCacheView(const BcDriveCacheTypes::BaseSets& driveSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BcDriveCacheViewMixins::Size(driveSets.Primary)
				, BcDriveCacheViewMixins::Contains(driveSets.Primary)
				, BcDriveCacheViewMixins::Iteration(driveSets.Primary)
				, BcDriveCacheViewMixins::ConstAccessor(driveSets.Primary)
				, BcDriveCacheViewMixins::PatriciaTreeView(driveSets.PatriciaTree.get())
				, BcDriveCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the drive cache.
	class BcDriveCacheView : public ReadOnlyViewSupplier<BasicBcDriveCacheView> {
	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit BcDriveCacheView(const BcDriveCacheTypes::BaseSets& driveSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}
