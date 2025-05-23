/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveBaseSets.h"
#include "DriveCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/ServiceConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive cache view.
	using DriveCacheViewMixins = PatriciaTreeCacheMixins<DriveCacheTypes::PrimaryTypes::BaseSetType, DriveCacheDescriptor>;

	/// Basic view on top of the drive cache.
	class BasicDriveCacheView
			: public utils::MoveOnly
			, public DriveCacheViewMixins::Size
			, public DriveCacheViewMixins::Contains
			, public DriveCacheViewMixins::Iteration
			, public DriveCacheViewMixins::ConstAccessor
			, public DriveCacheViewMixins::PatriciaTreeView
			, public DriveCacheViewMixins::ConfigBasedEnable<config::ServiceConfiguration> {
	public:
		using ReadOnlyView = DriveCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit BasicDriveCacheView(const DriveCacheTypes::BaseSets& driveSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: DriveCacheViewMixins::Size(driveSets.Primary)
				, DriveCacheViewMixins::Contains(driveSets.Primary)
				, DriveCacheViewMixins::Iteration(driveSets.Primary)
				, DriveCacheViewMixins::ConstAccessor(driveSets.Primary)
				, DriveCacheViewMixins::PatriciaTreeView(driveSets.PatriciaTree.get())
				, DriveCacheViewMixins::ConfigBasedEnable<config::ServiceConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the drive cache.
	class DriveCacheView : public ReadOnlyViewSupplier<BasicDriveCacheView> {
	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit DriveCacheView(const DriveCacheTypes::BaseSets& driveSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}
