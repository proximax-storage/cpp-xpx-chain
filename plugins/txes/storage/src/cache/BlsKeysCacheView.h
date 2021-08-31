/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BlsKeysBaseSets.h"
#include "BlsKeysCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the BLS keys cache view.
	using BlsKeysCacheViewMixins = PatriciaTreeCacheMixins<BlsKeysCacheTypes::PrimaryTypes::BaseSetType, BlsKeysCacheDescriptor>;

	/// Basic view on top of the BLS keys cache.
	class BasicBlsKeysCacheView
			: public utils::MoveOnly
			, public BlsKeysCacheViewMixins::Size
			, public BlsKeysCacheViewMixins::Contains
			, public BlsKeysCacheViewMixins::Iteration
			, public BlsKeysCacheViewMixins::ConstAccessor
			, public BlsKeysCacheViewMixins::PatriciaTreeView
			, public BlsKeysCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration> {
	public:
		using ReadOnlyView = BlsKeysCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a blsKeysSets and \a pConfigHolder.
		explicit BasicBlsKeysCacheView(const BlsKeysCacheTypes::BaseSets& blsKeysSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BlsKeysCacheViewMixins::Size(blsKeysSets.Primary)
				, BlsKeysCacheViewMixins::Contains(blsKeysSets.Primary)
				, BlsKeysCacheViewMixins::Iteration(blsKeysSets.Primary)
				, BlsKeysCacheViewMixins::ConstAccessor(blsKeysSets.Primary)
				, BlsKeysCacheViewMixins::PatriciaTreeView(blsKeysSets.PatriciaTree.get())
				, BlsKeysCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the BLS keys cache.
	class BlsKeysCacheView : public ReadOnlyViewSupplier<BasicBlsKeysCacheView> {
	public:
		/// Creates a view around \a blsKeysSets and \a pConfigHolder.
		explicit BlsKeysCacheView(const BlsKeysCacheTypes::BaseSets& blsKeysSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(blsKeysSets, pConfigHolder)
		{}
	};
}}
