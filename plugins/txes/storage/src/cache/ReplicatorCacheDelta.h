/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReplicatorBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive cache delta.
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

	/// Basic delta on top of the drive cache.
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
		/// Creates a delta around \a driveSets and \a pConfigHolder.
		explicit BasicReplicatorCacheDelta(
				const ReplicatorCacheTypes::BaseSetDeltaPointers& driveSets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReplicatorCacheDeltaMixins::Size(*driveSets.pPrimary)
				, ReplicatorCacheDeltaMixins::Contains(*driveSets.pPrimary)
				, ReplicatorCacheDeltaMixins::ConstAccessor(*driveSets.pPrimary)
				, ReplicatorCacheDeltaMixins::MutableAccessor(*driveSets.pPrimary)
				, ReplicatorCacheDeltaMixins::PatriciaTreeDelta(*driveSets.pPrimary, driveSets.pPatriciaTree)
				, ReplicatorCacheDeltaMixins::BasicInsertRemove(*driveSets.pPrimary)
				, ReplicatorCacheDeltaMixins::DeltaElements(*driveSets.pPrimary)
				, ReplicatorCacheDeltaMixins::ConfigBasedEnable(
						pConfigHolder, [](const auto& config) { return config.Enabled; })
						, m_pReplicatorEntries(driveSets.pPrimary)
						{}

	public:
		using ReplicatorCacheDeltaMixins::ConstAccessor::find;
		using ReplicatorCacheDeltaMixins::MutableAccessor::find;

	private:
		ReplicatorCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pReplicatorEntries;
	};

	/// Delta on top of the drive cache.
	class ReplicatorCacheDelta : public ReadOnlyViewSupplier<BasicReplicatorCacheDelta> {
	public:
		/// Creates a delta around \a driveSets and \a pConfigHolder.
		explicit ReplicatorCacheDelta(
				const ReplicatorCacheTypes::BaseSetDeltaPointers& driveSets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
				{}
	};
}}
