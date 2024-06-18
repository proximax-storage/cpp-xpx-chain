/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <utility>

#include "BootKeyReplicatorBaseSets.h"
#include "BootKeyReplicatorCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive cache view.
	using BootKeyReplicatorCacheViewMixins = PatriciaTreeCacheMixins<BootKeyReplicatorCacheTypes::PrimaryTypes::BaseSetType, BootKeyReplicatorCacheDescriptor>;

	/// Basic view on top of the drive cache.
	class BasicBootKeyReplicatorCacheView
		: public utils::MoveOnly
		, public BootKeyReplicatorCacheViewMixins::Size
		, public BootKeyReplicatorCacheViewMixins::Contains
		, public BootKeyReplicatorCacheViewMixins::Iteration
		, public BootKeyReplicatorCacheViewMixins::ConstAccessor
		, public BootKeyReplicatorCacheViewMixins::PatriciaTreeView
		, public BootKeyReplicatorCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration> {
	public:
		using ReadOnlyView = BootKeyReplicatorCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit BasicBootKeyReplicatorCacheView(const BootKeyReplicatorCacheTypes::BaseSets& driveSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: BootKeyReplicatorCacheViewMixins::Size(driveSets.Primary)
			, BootKeyReplicatorCacheViewMixins::Contains(driveSets.Primary)
			, BootKeyReplicatorCacheViewMixins::Iteration(driveSets.Primary)
			, BootKeyReplicatorCacheViewMixins::ConstAccessor(driveSets.Primary)
			, BootKeyReplicatorCacheViewMixins::PatriciaTreeView(driveSets.PatriciaTree.get())
			, BootKeyReplicatorCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration>(std::move(pConfigHolder), [](const auto& config) { return config.EnableReplicatorBootKeyBinding; })
		{}
	};

	/// View on top of the drive cache.
	class BootKeyReplicatorCacheView : public ReadOnlyViewSupplier<BasicBootKeyReplicatorCacheView> {
	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit BootKeyReplicatorCacheView(const BootKeyReplicatorCacheTypes::BaseSets& driveSets, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
		: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}