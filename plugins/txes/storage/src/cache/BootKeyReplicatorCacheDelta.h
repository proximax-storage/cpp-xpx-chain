/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <utility>

#include "BootKeyReplicatorBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	struct BootKeyReplicatorCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<BootKeyReplicatorCacheTypes::PrimaryTypes::BaseSetDeltaType, BootKeyReplicatorCacheDescriptor>;

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

	class BasicBootKeyReplicatorCacheDelta
			: public utils::MoveOnly
			, public BootKeyReplicatorCacheDeltaMixins::Size
			, public BootKeyReplicatorCacheDeltaMixins::Contains
			, public BootKeyReplicatorCacheDeltaMixins::ConstAccessor
			, public BootKeyReplicatorCacheDeltaMixins::MutableAccessor
			, public BootKeyReplicatorCacheDeltaMixins::PatriciaTreeDelta
			, public BootKeyReplicatorCacheDeltaMixins::BasicInsertRemove
			, public BootKeyReplicatorCacheDeltaMixins::DeltaElements
			, public BootKeyReplicatorCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = BootKeyReplicatorCacheTypes::CacheReadOnlyType;

	public:
		explicit BasicBootKeyReplicatorCacheDelta(
			const BootKeyReplicatorCacheTypes::BaseSetDeltaPointers& driveSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BootKeyReplicatorCacheDeltaMixins::Size(*driveSets.pPrimary)
				, BootKeyReplicatorCacheDeltaMixins::Contains(*driveSets.pPrimary)
				, BootKeyReplicatorCacheDeltaMixins::ConstAccessor(*driveSets.pPrimary)
				, BootKeyReplicatorCacheDeltaMixins::MutableAccessor(*driveSets.pPrimary)
				, BootKeyReplicatorCacheDeltaMixins::PatriciaTreeDelta(*driveSets.pPrimary, driveSets.pPatriciaTree)
				, BootKeyReplicatorCacheDeltaMixins::BasicInsertRemove(*driveSets.pPrimary)
				, BootKeyReplicatorCacheDeltaMixins::DeltaElements(*driveSets.pPrimary)
				, BootKeyReplicatorCacheDeltaMixins::ConfigBasedEnable(std::move(pConfigHolder), [](const auto& config) { return config.EnableReplicatorBootKeyBinding; })
				, m_pBootKeyReplicatorEntries(driveSets.pPrimary)
		{}

	public:
		using BootKeyReplicatorCacheDeltaMixins::ConstAccessor::find;
		using BootKeyReplicatorCacheDeltaMixins::MutableAccessor::find;

	private:
		BootKeyReplicatorCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pBootKeyReplicatorEntries;
	};

	class BootKeyReplicatorCacheDelta : public ReadOnlyViewSupplier<BasicBootKeyReplicatorCacheDelta> {
	public:
		explicit BootKeyReplicatorCacheDelta(
			const BootKeyReplicatorCacheTypes::BaseSetDeltaPointers& driveSets,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
				: ReadOnlyViewSupplier(driveSets, pConfigHolder)
		{}
	};
}}
