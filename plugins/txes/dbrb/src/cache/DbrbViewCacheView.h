/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <utility>

#include "DbrbViewBaseSets.h"
#include "DbrbViewCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/DbrbConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the view sequence cache view.
	using DbrbViewCacheViewMixins = PatriciaTreeCacheMixins<DbrbViewCacheTypes::PrimaryTypes::BaseSetType, DbrbViewCacheDescriptor>;

	/// Basic view on top of the view sequence cache.
	class BasicDbrbViewCacheView
			: public utils::MoveOnly
			, public DbrbViewCacheViewMixins::Size
			, public DbrbViewCacheViewMixins::Contains
			, public DbrbViewCacheViewMixins::Iteration
			, public DbrbViewCacheViewMixins::ConstAccessor
			, public DbrbViewCacheViewMixins::PatriciaTreeView
			, public DbrbViewCacheViewMixins::ConfigBasedEnable<config::DbrbConfiguration> {
	public:
		using ReadOnlyView = DbrbViewCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a dbrbViewSets and \a pConfigHolder.
		explicit BasicDbrbViewCacheView(const DbrbViewCacheTypes::BaseSets& dbrbViewSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: DbrbViewCacheViewMixins::Size(dbrbViewSets.Primary)
			, DbrbViewCacheViewMixins::Contains(dbrbViewSets.Primary)
			, DbrbViewCacheViewMixins::Iteration(dbrbViewSets.Primary)
			, DbrbViewCacheViewMixins::ConstAccessor(dbrbViewSets.Primary)
			, DbrbViewCacheViewMixins::PatriciaTreeView(dbrbViewSets.PatriciaTree.get())
			, DbrbViewCacheViewMixins::ConfigBasedEnable<config::DbrbConfiguration>(std::move(pConfigHolder), [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the view sequence cache.
	class DbrbViewCacheView : public ReadOnlyViewSupplier<BasicDbrbViewCacheView> {
	public:
		/// Creates a view around \a dbrbViewSets and \a pConfigHolder.
		explicit DbrbViewCacheView(const DbrbViewCacheTypes::BaseSets& dbrbViewSets, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
			: ReadOnlyViewSupplier(dbrbViewSets, pConfigHolder)
		{}
	};
}}
