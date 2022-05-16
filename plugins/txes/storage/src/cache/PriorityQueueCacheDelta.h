/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/config/StorageConfiguration.h"
#include "PriorityQueueBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the priority queue delta view.
	struct PriorityQueueCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<PriorityQueueCacheTypes::PrimaryTypes::BaseSetDeltaType, PriorityQueueCacheDescriptor>;

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

	/// Basic delta on top of the priority queue cache.
	class BasicPriorityQueueCacheDelta
			: public utils::MoveOnly
			, public PriorityQueueCacheDeltaMixins::Size
			, public PriorityQueueCacheDeltaMixins::Contains
			, public PriorityQueueCacheDeltaMixins::ConstAccessor
			, public PriorityQueueCacheDeltaMixins::MutableAccessor
			, public PriorityQueueCacheDeltaMixins::PatriciaTreeDelta
			, public PriorityQueueCacheDeltaMixins::BasicInsertRemove
			, public PriorityQueueCacheDeltaMixins::DeltaElements
			, public PriorityQueueCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = PriorityQueueCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a priorityQueueSets and \a pConfigHolder.
		explicit BasicPriorityQueueCacheDelta(
			const PriorityQueueCacheTypes::BaseSetDeltaPointers& priorityQueueSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: PriorityQueueCacheDeltaMixins::Size(*priorityQueueSets.pPrimary)
				, PriorityQueueCacheDeltaMixins::Contains(*priorityQueueSets.pPrimary)
				, PriorityQueueCacheDeltaMixins::ConstAccessor(*priorityQueueSets.pPrimary)
				, PriorityQueueCacheDeltaMixins::MutableAccessor(*priorityQueueSets.pPrimary)
				, PriorityQueueCacheDeltaMixins::PatriciaTreeDelta(*priorityQueueSets.pPrimary, priorityQueueSets.pPatriciaTree)
				, PriorityQueueCacheDeltaMixins::BasicInsertRemove(*priorityQueueSets.pPrimary)
				, PriorityQueueCacheDeltaMixins::DeltaElements(*priorityQueueSets.pPrimary)
				, PriorityQueueCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pPriorityQueueEntries(priorityQueueSets.pPrimary)
		{}

	public:
		using PriorityQueueCacheDeltaMixins::ConstAccessor::find;
		using PriorityQueueCacheDeltaMixins::MutableAccessor::find;

	private:
		PriorityQueueCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pPriorityQueueEntries;
	};

	/// Delta on top of the priority queue cache.
	class PriorityQueueCacheDelta : public ReadOnlyViewSupplier<BasicPriorityQueueCacheDelta> {
	public:
		/// Creates a delta around \a priorityQueueSets and \a pConfigHolder.
		explicit PriorityQueueCacheDelta(
			const PriorityQueueCacheTypes::BaseSetDeltaPointers& priorityQueueSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(priorityQueueSets, pConfigHolder)
		{}
	};
}}
