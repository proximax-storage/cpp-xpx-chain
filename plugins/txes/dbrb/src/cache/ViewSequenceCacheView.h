/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ViewSequenceBaseSets.h"
#include "ViewSequenceCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/DbrbConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the view sequence cache view.
	using ViewSequenceCacheViewMixins = PatriciaTreeCacheMixins<ViewSequenceCacheTypes::PrimaryTypes::BaseSetType, ViewSequenceCacheDescriptor>;

	/// Basic view on top of the view sequence cache.
	class BasicViewSequenceCacheView
			: public utils::MoveOnly
			, public ViewSequenceCacheViewMixins::Size
			, public ViewSequenceCacheViewMixins::Contains
			, public ViewSequenceCacheViewMixins::Iteration
			, public ViewSequenceCacheViewMixins::ConstAccessor
			, public ViewSequenceCacheViewMixins::PatriciaTreeView
			, public ViewSequenceCacheViewMixins::ConfigBasedEnable<config::DbrbConfiguration> {
	public:
		using ReadOnlyView = ViewSequenceCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a viewSequenceSets and \a pConfigHolder.
		explicit BasicViewSequenceCacheView(const ViewSequenceCacheTypes::BaseSets& viewSequenceSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ViewSequenceCacheViewMixins::Size(viewSequenceSets.Primary)
				, ViewSequenceCacheViewMixins::Contains(viewSequenceSets.Primary)
				, ViewSequenceCacheViewMixins::Iteration(viewSequenceSets.Primary)
				, ViewSequenceCacheViewMixins::ConstAccessor(viewSequenceSets.Primary)
				, ViewSequenceCacheViewMixins::PatriciaTreeView(viewSequenceSets.PatriciaTree.get())
				, ViewSequenceCacheViewMixins::ConfigBasedEnable<config::DbrbConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, ViewSequenceEntries(viewSequenceSets.Primary)
				, MessageHash(viewSequenceSets.MessageHash)
		{}

		dbrb::View getLatestView() {
			if (MessageHash.contains(0u)) {
				auto hash = MessageHash.find(0u).get()->hash();
				return ViewSequenceEntries.find(hash).get()->mostRecentView();
			}

			return {};
		}

	private:
		const ViewSequenceCacheTypes::PrimaryTypes::BaseSetType& ViewSequenceEntries;
		const ViewSequenceCacheTypes::MessageHashTypes::BaseSetType& MessageHash;
	};

	/// View on top of the view sequence cache.
	class ViewSequenceCacheView : public ReadOnlyViewSupplier<BasicViewSequenceCacheView> {
	public:
		/// Creates a view around \a viewSequenceSets and \a pConfigHolder.
		explicit ViewSequenceCacheView(const ViewSequenceCacheTypes::BaseSets& viewSequenceSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(viewSequenceSets, pConfigHolder)
		{}
	};
}}
