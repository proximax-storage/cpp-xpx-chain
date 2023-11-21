/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "PriorityQueueBaseSets.h"
#include "PriorityQueueCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the priority queue cache view.
	using PriorityQueueCacheViewMixins = PatriciaTreeCacheMixins<PriorityQueueCacheTypes::PrimaryTypes::BaseSetType, PriorityQueueCacheDescriptor>;

	/// Basic view on top of the priority queue cache.
	class BasicPriorityQueueCacheView
			: public utils::MoveOnly
			, public PriorityQueueCacheViewMixins::Size
			, public PriorityQueueCacheViewMixins::Contains
			, public PriorityQueueCacheViewMixins::Iteration
			, public PriorityQueueCacheViewMixins::ConstAccessor
			, public PriorityQueueCacheViewMixins::PatriciaTreeView
			, public PriorityQueueCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration> {
	public:
		using ReadOnlyView = PriorityQueueCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a priorityQueueSets and \a pConfigHolder.
		explicit BasicPriorityQueueCacheView(const PriorityQueueCacheTypes::BaseSets& priorityQueueSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: PriorityQueueCacheViewMixins::Size(priorityQueueSets.Primary)
				, PriorityQueueCacheViewMixins::Contains(priorityQueueSets.Primary)
				, PriorityQueueCacheViewMixins::Iteration(priorityQueueSets.Primary)
				, PriorityQueueCacheViewMixins::ConstAccessor(priorityQueueSets.Primary)
				, PriorityQueueCacheViewMixins::PatriciaTreeView(priorityQueueSets.PatriciaTree.get())
				, PriorityQueueCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the priority queue cache.
	class PriorityQueueCacheView : public ReadOnlyViewSupplier<BasicPriorityQueueCacheView> {
	public:
		/// Creates a view around \a priorityQueueSets and \a pConfigHolder.
		explicit PriorityQueueCacheView(const PriorityQueueCacheTypes::BaseSets& priorityQueueSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(priorityQueueSets, pConfigHolder)
		{}
	};
}}
