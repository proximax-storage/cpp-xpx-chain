/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultUpgradeBaseSets.h"
#include "CatapultUpgradeCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the catapult upgrade cache view.
	using CatapultUpgradeCacheViewMixins = PatriciaTreeCacheMixins<CatapultUpgradeCacheTypes::PrimaryTypes::BaseSetType, CatapultUpgradeCacheDescriptor>;

	/// Basic view on top of the catapult upgrade cache.
	class BasicCatapultUpgradeCacheView
			: public utils::MoveOnly
			, public CatapultUpgradeCacheViewMixins::Size
			, public CatapultUpgradeCacheViewMixins::Contains
			, public CatapultUpgradeCacheViewMixins::Iteration
			, public CatapultUpgradeCacheViewMixins::ConstAccessor
			, public CatapultUpgradeCacheViewMixins::PatriciaTreeView
			, public CatapultUpgradeCacheViewMixins::Enable {
	public:
		using ReadOnlyView = CatapultUpgradeCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a catapultUpgradeSets.
		explicit BasicCatapultUpgradeCacheView(const CatapultUpgradeCacheTypes::BaseSets& catapultUpgradeSets)
				: CatapultUpgradeCacheViewMixins::Size(catapultUpgradeSets.Primary)
				, CatapultUpgradeCacheViewMixins::Contains(catapultUpgradeSets.Primary)
				, CatapultUpgradeCacheViewMixins::Iteration(catapultUpgradeSets.Primary)
				, CatapultUpgradeCacheViewMixins::ConstAccessor(catapultUpgradeSets.Primary)
				, CatapultUpgradeCacheViewMixins::PatriciaTreeView(catapultUpgradeSets.PatriciaTree.get())
		{}
	};

	/// View on top of the catapult upgrade cache.
	class CatapultUpgradeCacheView : public ReadOnlyViewSupplier<BasicCatapultUpgradeCacheView> {
	public:
		/// Creates a view around \a catapultUpgradeSets.
		explicit CatapultUpgradeCacheView(const CatapultUpgradeCacheTypes::BaseSets& catapultUpgradeSets)
				: ReadOnlyViewSupplier(catapultUpgradeSets)
		{}
	};
}}
