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
			, public CatapultConfigCacheViewMixins::PatriciaTreeView
			, public CatapultConfigCacheViewMixins::Enable
			, public CatapultConfigCacheViewMixins::Height {
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
				, m_catapultConfigHeights(catapultConfigSets.Heights)
		{}

	public:
		using CatapultConfigCacheViewMixins::ConstAccessor::find;

	public:
		Height FindConfigHeightAt(const Height& height) const {
			if (!height.unwrap())
				return height;

			auto iterableView = MakeIterableView(m_catapultConfigHeights);
			auto iter = std::lower_bound(iterableView.begin(), iterableView.end(), height);
			if (iter == iterableView.end()) {
				if (iter == iterableView.begin())
					return Height{0};
				iter--;
			} else {
				if (*iter > height) {
					if (iter == iterableView.begin())
						return Height{0};
					iter--;
				}
			}

			return *iter;
		}

	private:
		const CatapultConfigCacheTypes::HeightTypes::BaseSetType& m_catapultConfigHeights;
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
