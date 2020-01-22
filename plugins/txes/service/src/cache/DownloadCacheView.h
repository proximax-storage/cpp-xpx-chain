/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadBaseSets.h"
#include "DownloadCacheSerializers.h"
#include "ServiceCacheTools.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Mixins used by the download cache view.
	using DownloadCacheViewMixins = PatriciaTreeCacheMixins<DownloadCacheTypes::PrimaryTypes::BaseSetType, DownloadCacheDescriptor>;

	/// Basic view on top of the download cache.
	class BasicDownloadCacheView
			: public utils::MoveOnly
			, public DownloadCacheViewMixins::Size
			, public DownloadCacheViewMixins::Contains
			, public DownloadCacheViewMixins::Iteration
			, public DownloadCacheViewMixins::ConstAccessor
			, public DownloadCacheViewMixins::PatriciaTreeView
			, public DownloadCacheViewMixins::Enable
			, public DownloadCacheViewMixins::Height {
	public:
		using ReadOnlyView = DownloadCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a downloadSets and \a pConfigHolder.
		explicit BasicDownloadCacheView(
			const DownloadCacheTypes::BaseSets& downloadSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: DownloadCacheViewMixins::Size(downloadSets.Primary)
				, DownloadCacheViewMixins::Contains(downloadSets.Primary)
				, DownloadCacheViewMixins::Iteration(downloadSets.Primary)
				, DownloadCacheViewMixins::ConstAccessor(downloadSets.Primary)
				, DownloadCacheViewMixins::PatriciaTreeView(downloadSets.PatriciaTree.get())
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		bool enabled() const {
			return DownloadCacheEnabled(m_pConfigHolder, height());
		}

	private:
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// View on top of the download cache.
	class DownloadCacheView : public ReadOnlyViewSupplier<BasicDownloadCacheView> {
	public:
		/// Creates a view around \a downloadSets and \a pConfigHolder.
		explicit DownloadCacheView(
			const DownloadCacheTypes::BaseSets& downloadSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(downloadSets, pConfigHolder)
		{}
	};
}}
