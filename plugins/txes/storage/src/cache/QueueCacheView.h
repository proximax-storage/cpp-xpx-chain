/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "QueueBaseSets.h"
#include "QueueCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive cache view.
	using QueueCacheViewMixins = PatriciaTreeCacheMixins<QueueCacheTypes::PrimaryTypes::BaseSetType, QueueCacheDescriptor>;

	/// Basic view on top of the drive cache.
	class BasicQueueCacheView
			: public utils::MoveOnly
			, public QueueCacheViewMixins::Size
			, public QueueCacheViewMixins::Contains
			, public QueueCacheViewMixins::Iteration
			, public QueueCacheViewMixins::ConstAccessor
			, public QueueCacheViewMixins::PatriciaTreeView
			, public QueueCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration> {
	public:
		using ReadOnlyView = QueueCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit BasicQueueCacheView(const QueueCacheTypes::BaseSets& driveSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: QueueCacheViewMixins::Size(driveSets.Primary)
				, QueueCacheViewMixins::Contains(driveSets.Primary)
				, QueueCacheViewMixins::Iteration(driveSets.Primary)
				, QueueCacheViewMixins::ConstAccessor(driveSets.Primary)
				, QueueCacheViewMixins::PatriciaTreeView(driveSets.PatriciaTree.get())
				, QueueCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the drive cache.
	class QueueCacheView : public ReadOnlyViewSupplier<BasicQueueCacheView> {
	public:
		/// Creates a view around \a driveSets and \a pConfigHolder.
		explicit QueueCacheView(const QueueCacheTypes::BaseSets& driveSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}
