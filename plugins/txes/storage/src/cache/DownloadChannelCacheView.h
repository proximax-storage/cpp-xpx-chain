/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadChannelBaseSets.h"
#include "DownloadChannelCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace cache {

	/// Mixins used by the download cache view.
	using DownloadChannelCacheViewMixins = PatriciaTreeCacheMixins<DownloadChannelCacheTypes::PrimaryTypes::BaseSetType, DownloadChannelCacheDescriptor>;

	/// Basic view on top of the download cache.
	class BasicDownloadChannelCacheView
			: public utils::MoveOnly
			, public DownloadChannelCacheViewMixins::Size
			, public DownloadChannelCacheViewMixins::Contains
			, public DownloadChannelCacheViewMixins::Iteration
			, public DownloadChannelCacheViewMixins::ConstAccessor
			, public DownloadChannelCacheViewMixins::PatriciaTreeView
			, public DownloadChannelCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration> {
	public:
		using ReadOnlyView = DownloadChannelCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a downloadSets and \a pConfigHolder.
		explicit BasicDownloadChannelCacheView(const DownloadChannelCacheTypes::BaseSets& downloadSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: DownloadChannelCacheViewMixins::Size(downloadSets.Primary)
				, DownloadChannelCacheViewMixins::Contains(downloadSets.Primary)
				, DownloadChannelCacheViewMixins::Iteration(downloadSets.Primary)
				, DownloadChannelCacheViewMixins::ConstAccessor(downloadSets.Primary)
				, DownloadChannelCacheViewMixins::PatriciaTreeView(downloadSets.PatriciaTree.get())
				, DownloadChannelCacheViewMixins::ConfigBasedEnable<config::StorageConfiguration>(pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// View on top of the download cache.
	class DownloadChannelCacheView : public ReadOnlyViewSupplier<BasicDownloadChannelCacheView> {
	public:
		/// Creates a view around \a downloadSets and \a pConfigHolder.
		explicit DownloadChannelCacheView(const DownloadChannelCacheTypes::BaseSets& downloadSets, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(downloadSets, pConfigHolder)
		{}
	};
}}
