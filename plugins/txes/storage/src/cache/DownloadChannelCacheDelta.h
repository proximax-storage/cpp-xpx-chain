/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/config/StorageConfiguration.h"
#include "DownloadChannelBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the download delta view.
	struct DownloadChannelCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<DownloadChannelCacheTypes::PrimaryTypes::BaseSetDeltaType, DownloadChannelCacheDescriptor>;

	public:
		using Size = PrimaryMixins::Size;
		using Contains = PrimaryMixins::Contains;
		using PatriciaTreeDelta = PrimaryMixins::PatriciaTreeDelta;
		using MutableAccessor = PrimaryMixins::ConstAccessor;
		using ConstAccessor = PrimaryMixins::MutableAccessor;
		using DeltaElements = PrimaryMixins::DeltaElements;
		using BasicInsertRemove = PrimaryMixins::BasicInsertRemove;
		using ConfigBasedEnable = PrimaryMixins::ConfigBasedEnable<config::StorageConfiguration>;
	};

	/// Basic delta on top of the download cache.
	class BasicDownloadChannelCacheDelta
			: public utils::MoveOnly
			, public DownloadChannelCacheDeltaMixins::Size
			, public DownloadChannelCacheDeltaMixins::Contains
			, public DownloadChannelCacheDeltaMixins::ConstAccessor
			, public DownloadChannelCacheDeltaMixins::MutableAccessor
			, public DownloadChannelCacheDeltaMixins::PatriciaTreeDelta
			, public DownloadChannelCacheDeltaMixins::BasicInsertRemove
			, public DownloadChannelCacheDeltaMixins::DeltaElements
			, public DownloadChannelCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = DownloadChannelCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a downloadSets and \a pConfigHolder.
		explicit BasicDownloadChannelCacheDelta(
			const DownloadChannelCacheTypes::BaseSetDeltaPointers& downloadSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: DownloadChannelCacheDeltaMixins::Size(*downloadSets.pPrimary)
				, DownloadChannelCacheDeltaMixins::Contains(*downloadSets.pPrimary)
				, DownloadChannelCacheDeltaMixins::ConstAccessor(*downloadSets.pPrimary)
				, DownloadChannelCacheDeltaMixins::MutableAccessor(*downloadSets.pPrimary)
				, DownloadChannelCacheDeltaMixins::PatriciaTreeDelta(*downloadSets.pPrimary, downloadSets.pPatriciaTree)
				, DownloadChannelCacheDeltaMixins::BasicInsertRemove(*downloadSets.pPrimary)
				, DownloadChannelCacheDeltaMixins::DeltaElements(*downloadSets.pPrimary)
				, DownloadChannelCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pDownloadEntries(downloadSets.pPrimary)
		{}

	public:
		using DownloadChannelCacheDeltaMixins::ConstAccessor::find;
		using DownloadChannelCacheDeltaMixins::MutableAccessor::find;

	private:
		DownloadChannelCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pDownloadEntries;
	};

	/// Delta on top of the download cache.
	class DownloadChannelCacheDelta : public ReadOnlyViewSupplier<BasicDownloadChannelCacheDelta> {
	public:
		/// Creates a delta around \a downloadSets and \a pConfigHolder.
		explicit DownloadChannelCacheDelta(
			const DownloadChannelCacheTypes::BaseSetDeltaPointers& downloadSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(downloadSets, pConfigHolder)
		{}
	};
}}
