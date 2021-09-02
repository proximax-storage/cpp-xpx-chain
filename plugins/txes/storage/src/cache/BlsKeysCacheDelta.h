/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/config/StorageConfiguration.h"
#include "BlsKeysBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the BLS keys delta view.
	struct BlsKeysCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<BlsKeysCacheTypes::PrimaryTypes::BaseSetDeltaType, BlsKeysCacheDescriptor>;

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

	/// Basic delta on top of the BLS keys cache.
	class BasicBlsKeysCacheDelta
			: public utils::MoveOnly
			, public BlsKeysCacheDeltaMixins::Size
			, public BlsKeysCacheDeltaMixins::Contains
			, public BlsKeysCacheDeltaMixins::ConstAccessor
			, public BlsKeysCacheDeltaMixins::MutableAccessor
			, public BlsKeysCacheDeltaMixins::PatriciaTreeDelta
			, public BlsKeysCacheDeltaMixins::BasicInsertRemove
			, public BlsKeysCacheDeltaMixins::DeltaElements
			, public BlsKeysCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = BlsKeysCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a blsKeysSets and \a pConfigHolder.
		explicit BasicBlsKeysCacheDelta(
			const BlsKeysCacheTypes::BaseSetDeltaPointers& blsKeysSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BlsKeysCacheDeltaMixins::Size(*blsKeysSets.pPrimary)
				, BlsKeysCacheDeltaMixins::Contains(*blsKeysSets.pPrimary)
				, BlsKeysCacheDeltaMixins::ConstAccessor(*blsKeysSets.pPrimary)
				, BlsKeysCacheDeltaMixins::MutableAccessor(*blsKeysSets.pPrimary)
				, BlsKeysCacheDeltaMixins::PatriciaTreeDelta(*blsKeysSets.pPrimary, blsKeysSets.pPatriciaTree)
				, BlsKeysCacheDeltaMixins::BasicInsertRemove(*blsKeysSets.pPrimary)
				, BlsKeysCacheDeltaMixins::DeltaElements(*blsKeysSets.pPrimary)
				, BlsKeysCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pBlsKeysEntries(blsKeysSets.pPrimary)
		{}

	public:
		using BlsKeysCacheDeltaMixins::ConstAccessor::find;
		using BlsKeysCacheDeltaMixins::MutableAccessor::find;

	private:
		BlsKeysCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pBlsKeysEntries;
	};

	/// Delta on top of the BLS keys cache.
	class BlsKeysCacheDelta : public ReadOnlyViewSupplier<BasicBlsKeysCacheDelta> {
	public:
		/// Creates a delta around \a blsKeysSets and \a pConfigHolder.
		explicit BlsKeysCacheDelta(
			const BlsKeysCacheTypes::BaseSetDeltaPointers& blsKeysSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(blsKeysSets, pConfigHolder)
		{}
	};
}}
