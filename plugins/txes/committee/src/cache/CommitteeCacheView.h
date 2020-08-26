/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeBaseSets.h"
#include "CommitteeCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the committee cache view.
	using CommitteeCacheViewMixins = PatriciaTreeCacheMixins<CommitteeCacheTypes::PrimaryTypes::BaseSetType, CommitteeCacheDescriptor>;

	/// Basic view on top of the committee cache.
	class BasicCommitteeCacheView
			: public utils::MoveOnly
			, public CommitteeCacheViewMixins::Size
			, public CommitteeCacheViewMixins::Contains
			, public CommitteeCacheViewMixins::Iteration
			, public CommitteeCacheViewMixins::ConstAccessor
			, public CommitteeCacheViewMixins::PatriciaTreeView
			, public CommitteeCacheDeltaMixins::ConfigBasedEnable<config::CommitteeConfiguration> {
	public:
		using ReadOnlyView = CommitteeCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a committeeSets.
		explicit BasicCommitteeCacheView(
			const CommitteeCacheTypes::BaseSets& committeeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: CommitteeCacheViewMixins::Size(committeeSets.Primary)
				, CommitteeCacheViewMixins::Contains(committeeSets.Primary)
				, CommitteeCacheViewMixins::Iteration(committeeSets.Primary)
				, CommitteeCacheViewMixins::ConstAccessor(committeeSets.Primary)
				, CommitteeCacheViewMixins::PatriciaTreeView(committeeSets.PatriciaTree.get())
				, CommitteeCacheDeltaMixins::ConfigBasedEnable<config::CommitteeConfiguration>(
					pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the committee cache.
	class CommitteeCacheView : public ReadOnlyViewSupplier<BasicCommitteeCacheView> {
	public:
		/// Creates a view around \a committeeSets.
		explicit CommitteeCacheView(
			const CommitteeCacheTypes::BaseSets& committeeSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(committeeSets, pConfigHolder)
		{}
	};
}}
