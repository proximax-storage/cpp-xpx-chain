/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultConfigBaseSets.h"
#include "CatapultConfigCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the catapult config cache view.
	using CatapultConfigCacheViewMixins = PatriciaTreeCacheMixins<CatapultConfigCacheTypes::PrimaryTypes::BaseSetType, CatapultConfigCacheDescriptor>;

	/// Basic view on top of the catapult config cache.
	class BasicCatapultConfigCacheView
			: public utils::MoveOnly
			, public CatapultConfigCacheViewMixins::Size
			, public CatapultConfigCacheViewMixins::Contains
			, public CatapultConfigCacheViewMixins::Iteration
			, public CatapultConfigCacheViewMixins::ConstAccessor
			, public CatapultConfigCacheViewMixins::PatriciaTreeView {
	public:
		using ReadOnlyView = CatapultConfigCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a catapultConfigSets.
		explicit BasicCatapultConfigCacheView(const CatapultConfigCacheTypes::BaseSets& catapultConfigSets)
				: CatapultConfigCacheViewMixins::Size(catapultConfigSets.Primary)
				, CatapultConfigCacheViewMixins::Contains(catapultConfigSets.Primary)
				, CatapultConfigCacheViewMixins::Iteration(catapultConfigSets.Primary)
				, CatapultConfigCacheViewMixins::ConstAccessor(catapultConfigSets.Primary)
				, CatapultConfigCacheViewMixins::PatriciaTreeView(catapultConfigSets.PatriciaTree.get())
		{}
	};

	/// View on top of the catapult config cache.
	class CatapultConfigCacheView : public ReadOnlyViewSupplier<BasicCatapultConfigCacheView> {
	public:
		/// Creates a view around \a catapultConfigSets.
		explicit CatapultConfigCacheView(const CatapultConfigCacheTypes::BaseSets& catapultConfigSets)
				: ReadOnlyViewSupplier(catapultConfigSets)
		{}
	};
}}
