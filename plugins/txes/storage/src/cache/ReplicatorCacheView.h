/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReplicatorBaseSets.h"
#include "ReplicatorCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive cache view.
	using ReplicatorCacheViewMixins = PatriciaTreeCacheMixins<ReplicatorCacheTypes::PrimaryTypes::BaseSetType, ReplicatorCacheDescriptor>;

	/// Basic view on top of the drive cache.
	class BasicReplicatorCacheView
			: public utils::MoveOnly
			, public ReplicatorCacheViewMixins::Size
			, public ReplicatorCacheViewMixins::Contains
			, public ReplicatorCacheViewMixins::Iteration
			, public ReplicatorCacheViewMixins::ConstAccessor
			, public ReplicatorCacheViewMixins::PatriciaTreeView
			, public ReplicatorCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration> {
	public:
		using ReadOnlyView = ReplicatorCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit BasicReplicatorCacheView(const ReplicatorCacheTypes::BaseSets& driveSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
		: ReplicatorCacheViewMixins::Size(driveSets.Primary)
		, ReplicatorCacheViewMixins::Contains(driveSets.Primary)
		, ReplicatorCacheViewMixins::Iteration(driveSets.Primary)
		, ReplicatorCacheViewMixins::ConstAccessor(driveSets.Primary)
		, ReplicatorCacheViewMixins::PatriciaTreeView(driveSets.PatriciaTree.get())
		, ReplicatorCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the drive cache.
	class ReplicatorCacheView : public ReadOnlyViewSupplier<BasicReplicatorCacheView> {
	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit ReplicatorCacheView(const ReplicatorCacheTypes::BaseSets& driveSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
		: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}