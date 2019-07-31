/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReputationBaseSets.h"
#include "ReputationCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the reputation cache view.
	using ReputationCacheViewMixins = PatriciaTreeCacheMixins<ReputationCacheTypes::PrimaryTypes::BaseSetType, ReputationCacheDescriptor>;

	/// Basic view on top of the reputation cache.
	class BasicReputationCacheView
			: public utils::MoveOnly
			, public ReputationCacheViewMixins::Size
			, public ReputationCacheViewMixins::Contains
			, public ReputationCacheViewMixins::Iteration
			, public ReputationCacheViewMixins::ConstAccessor
			, public ReputationCacheViewMixins::PatriciaTreeView
			, public ReputationCacheViewMixins::Enable
			, public ReputationCacheViewMixins::Height {
	public:
		using ReadOnlyView = ReputationCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a reputationSets.
		explicit BasicReputationCacheView(const ReputationCacheTypes::BaseSets& reputationSets)
				: ReputationCacheViewMixins::Size(reputationSets.Primary)
				, ReputationCacheViewMixins::Contains(reputationSets.Primary)
				, ReputationCacheViewMixins::Iteration(reputationSets.Primary)
				, ReputationCacheViewMixins::ConstAccessor(reputationSets.Primary)
				, ReputationCacheViewMixins::PatriciaTreeView(reputationSets.PatriciaTree.get())
		{}
	};

	/// View on top of the reputation cache.
	class ReputationCacheView : public ReadOnlyViewSupplier<BasicReputationCacheView> {
	public:
		/// Creates a view around \a reputationSets.
		explicit ReputationCacheView(const ReputationCacheTypes::BaseSets& reputationSets)
				: ReadOnlyViewSupplier(reputationSets)
		{}
	};
}}
