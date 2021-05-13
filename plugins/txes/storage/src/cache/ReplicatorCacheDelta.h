/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/config/StorageConfiguration.h"
#include "ReplicatorBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the replicator delta view.
	struct ReplicatorCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<ReplicatorCacheTypes::PrimaryTypes::BaseSetDeltaType, ReplicatorCacheDescriptor>;

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

	/// Basic delta on top of the replicator cache.
	class BasicReplicatorCacheDelta
			: public utils::MoveOnly
			, public ReplicatorCacheDeltaMixins::Size
			, public ReplicatorCacheDeltaMixins::Contains
			, public ReplicatorCacheDeltaMixins::ConstAccessor
			, public ReplicatorCacheDeltaMixins::MutableAccessor
			, public ReplicatorCacheDeltaMixins::PatriciaTreeDelta
			, public ReplicatorCacheDeltaMixins::BasicInsertRemove
			, public ReplicatorCacheDeltaMixins::DeltaElements
			, public ReplicatorCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = ReplicatorCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a replicatorSets and \a pConfigHolder.
		explicit BasicReplicatorCacheDelta(
			const ReplicatorCacheTypes::BaseSetDeltaPointers& replicatorSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReplicatorCacheDeltaMixins::Size(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::Contains(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::ConstAccessor(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::MutableAccessor(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::PatriciaTreeDelta(*replicatorSets.pPrimary, replicatorSets.pPatriciaTree)
				, ReplicatorCacheDeltaMixins::BasicInsertRemove(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::DeltaElements(*replicatorSets.pPrimary)
				, ReplicatorCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pReplicatorEntries(replicatorSets.pPrimary)
		{}

	public:
		using ReplicatorCacheDeltaMixins::ConstAccessor::find;
		using ReplicatorCacheDeltaMixins::MutableAccessor::find;

	private:
		ReplicatorCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pReplicatorEntries;
	};

	/// Delta on top of the replicator cache.
	class ReplicatorCacheDelta : public ReadOnlyViewSupplier<BasicReplicatorCacheDelta> {
	public:
		/// Creates a delta around \a replicatorSets and \a pConfigHolder.
		explicit ReplicatorCacheDelta(
			const ReplicatorCacheTypes::BaseSetDeltaPointers& replicatorSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(replicatorSets, pConfigHolder)
		{}
	};
}}
