/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#pragma once
#include "GlobalStoreBaseSets.h"
#include "GlobalStoreCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the global store ache view.
	struct GlobalStoreCacheViewMixins {
	public:
		using PrimaryMixins = PatriciaTreeCacheMixins<GlobalStoreCacheTypes::PrimaryTypes::BaseSetType, GlobalStoreCacheDescriptor>;
		using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::GlobalStoreConfiguration>;
	};
	/// Basic view on top of the global store cache.
	class BasicGlobalStoreCacheView
			: public utils::MoveOnly
			, public GlobalStoreCacheViewMixins::PrimaryMixins::Size
			, public GlobalStoreCacheViewMixins::PrimaryMixins::Contains
			, public GlobalStoreCacheViewMixins::PrimaryMixins::Iteration
			, public GlobalStoreCacheViewMixins::PrimaryMixins::ConstAccessor
			, public GlobalStoreCacheViewMixins::PrimaryMixins::PatriciaTreeView
			, public GlobalStoreCacheViewMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = GlobalStoreCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a globalentrywSets and \a networkIdentifier.
		BasicGlobalStoreCacheView(
				const GlobalStoreCacheTypes::BaseSets& globalEntrySets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: GlobalStoreCacheViewMixins::PrimaryMixins::Size(globalEntrySets.Primary)
				, GlobalStoreCacheViewMixins::PrimaryMixins::Contains(globalEntrySets.Primary)
				, GlobalStoreCacheViewMixins::PrimaryMixins::Iteration(globalEntrySets.Primary)
				, GlobalStoreCacheViewMixins::PrimaryMixins::ConstAccessor(globalEntrySets.Primary)
				, GlobalStoreCacheViewMixins::PrimaryMixins::PatriciaTreeView(globalEntrySets.PatriciaTree.get())
				, GlobalStoreCacheViewMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_networkIdentifier(pConfigHolder->Config().Immutable.NetworkIdentifier)
		{}

	public:
		/// Gets the network identifier.
		model::NetworkIdentifier networkIdentifier() const {
			return m_networkIdentifier;
		}

	private:
		model::NetworkIdentifier m_networkIdentifier;
	};

	/// View on top of the account restriction cache.
	class GlobalStoreCacheView : public ReadOnlyViewSupplier<BasicGlobalStoreCacheView> {
	public:
		/// Creates a view around \a restrictionSets and \a networkIdentifier.
		GlobalStoreCacheView(
				const GlobalStoreCacheTypes::BaseSets& restrictionSets,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(restrictionSets, pConfigHolder)
		{}
	};
}}
