/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/config/StorageConfiguration.h"
#include "DownloadBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the download delta view.
	struct DownloadCacheDeltaMixins {
	private:
		using PrimaryMixins = PatriciaTreeCacheMixins<DownloadCacheTypes::PrimaryTypes::BaseSetDeltaType, DownloadCacheDescriptor>;

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
	class BasicDownloadCacheDelta
			: public utils::MoveOnly
			, public DownloadCacheDeltaMixins::Size
			, public DownloadCacheDeltaMixins::Contains
			, public DownloadCacheDeltaMixins::ConstAccessor
			, public DownloadCacheDeltaMixins::MutableAccessor
			, public DownloadCacheDeltaMixins::PatriciaTreeDelta
			, public DownloadCacheDeltaMixins::BasicInsertRemove
			, public DownloadCacheDeltaMixins::DeltaElements
			, public DownloadCacheDeltaMixins::ConfigBasedEnable {
	public:
		using ReadOnlyView = DownloadCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a downloadSets and \a pConfigHolder.
		explicit BasicDownloadCacheDelta(
			const DownloadCacheTypes::BaseSetDeltaPointers& downloadSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: DownloadCacheDeltaMixins::Size(*downloadSets.pPrimary)
				, DownloadCacheDeltaMixins::Contains(*downloadSets.pPrimary)
				, DownloadCacheDeltaMixins::ConstAccessor(*downloadSets.pPrimary)
				, DownloadCacheDeltaMixins::MutableAccessor(*downloadSets.pPrimary)
				, DownloadCacheDeltaMixins::PatriciaTreeDelta(*downloadSets.pPrimary, downloadSets.pPatriciaTree)
				, DownloadCacheDeltaMixins::BasicInsertRemove(*downloadSets.pPrimary)
				, DownloadCacheDeltaMixins::DeltaElements(*downloadSets.pPrimary)
				, DownloadCacheDeltaMixins::ConfigBasedEnable(pConfigHolder, [](const auto& config) { return config.Enabled; })
				, m_pDownloadEntries(downloadSets.pPrimary)
		{}

	public:
		using DownloadCacheDeltaMixins::ConstAccessor::find;
		using DownloadCacheDeltaMixins::MutableAccessor::find;

	private:
		DownloadCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pDownloadEntries;
	};

	/// Delta on top of the download cache.
	class DownloadCacheDelta : public ReadOnlyViewSupplier<BasicDownloadCacheDelta> {
	public:
		/// Creates a delta around \a downloadSets and \a pConfigHolder.
		explicit DownloadCacheDelta(
			const DownloadCacheTypes::BaseSetDeltaPointers& downloadSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(downloadSets, pConfigHolder)
		{}
	};
}}
