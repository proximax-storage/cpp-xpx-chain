/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#pragma once
#include "../config/GlobalStoreConfiguration.h"
#include "GlobalStoreBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include "catapult/model/NetworkInfo.h"

namespace catapult { namespace cache {

	/// Mixins used by the global store cache delta.
	struct GlobalStoreCacheDeltaMixins {
	public:
		using PrimaryMixins = PatriciaTreeCacheMixins<GlobalStoreCacheTypes::PrimaryTypes::BaseSetDeltaType, GlobalStoreCacheDescriptor>;
		using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::GlobalStoreConfiguration>;
	};

	/// Basic delta on top of the global store cache.
	class BasicGlobalStoreCacheDelta
			: public utils::MoveOnly
			, public GlobalStoreCacheDeltaMixins::PrimaryMixins::Size
			, public GlobalStoreCacheDeltaMixins::PrimaryMixins::Contains
			, public GlobalStoreCacheDeltaMixins::PrimaryMixins::ConstAccessor
			, public GlobalStoreCacheDeltaMixins::PrimaryMixins::MutableAccessor
			, public GlobalStoreCacheDeltaMixins::PrimaryMixins::PatriciaTreeDelta
			, public GlobalStoreCacheDeltaMixins::PrimaryMixins::BasicInsertRemove
			, public GlobalStoreCacheDeltaMixins::PrimaryMixins::DeltaElements
			, public GlobalStoreCacheDeltaMixins::PrimaryMixins::BroadIteration
			, public GlobalStoreCacheDeltaMixins::ConfigBasedEnable{
	public:
		using ReadOnlyView = GlobalStoreCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a globalEntrySets and \a networkIdentifier.
		BasicGlobalStoreCacheDelta(
				const GlobalStoreCacheTypes::BaseSetDeltaPointers& globalEntrySets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: GlobalStoreCacheDeltaMixins::PrimaryMixins::Size(*globalEntrySets.pPrimary)
				, GlobalStoreCacheDeltaMixins::PrimaryMixins::Contains(*globalEntrySets.pPrimary)
				, GlobalStoreCacheDeltaMixins::PrimaryMixins::ConstAccessor(*globalEntrySets.pPrimary)
				, GlobalStoreCacheDeltaMixins::PrimaryMixins::MutableAccessor(*globalEntrySets.pPrimary)
				, GlobalStoreCacheDeltaMixins::PrimaryMixins::PatriciaTreeDelta(*globalEntrySets.pPrimary, globalEntrySets.pPatriciaTree)
				, GlobalStoreCacheDeltaMixins::PrimaryMixins::BasicInsertRemove(*globalEntrySets.pPrimary)
				, GlobalStoreCacheDeltaMixins::PrimaryMixins::DeltaElements(*globalEntrySets.pPrimary)
				, GlobalStoreCacheDeltaMixins::PrimaryMixins::BroadIteration (*globalEntrySets.pPrimary)
				, GlobalStoreCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pGlobalStoreEntries(globalEntrySets.pPrimary)
				, m_networkIdentifier(pConfigHolder->Config().Immutable.NetworkIdentifier)
		{}

	public:
		using GlobalStoreCacheDeltaMixins::PrimaryMixins::ConstAccessor::find;
		using GlobalStoreCacheDeltaMixins::PrimaryMixins::MutableAccessor::find;

	public:
		/// Gets the network identifier.
		model::NetworkIdentifier networkIdentifier() const {
			return m_networkIdentifier;
		}

	private:
		GlobalStoreCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pGlobalStoreEntries;
		model::NetworkIdentifier m_networkIdentifier;
	};

	/// Delta on top of the global store cache.
	class GlobalStoreCacheDelta : public ReadOnlyViewSupplier<BasicGlobalStoreCacheDelta> {
	public:
		/// Creates a delta around \a globalEntrySets and \a networkIdentifier.
		GlobalStoreCacheDelta(
				const GlobalStoreCacheTypes::BaseSetDeltaPointers& entrySets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(entrySets, pConfigHolder)
		{}
	};
}}
