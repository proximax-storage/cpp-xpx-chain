/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadBaseSets.h"
#include "ServiceCacheTools.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the download cache delta.
	using DownloadCacheDeltaMixins = PatriciaTreeCacheMixins<DownloadCacheTypes::PrimaryTypes::BaseSetDeltaType, DownloadCacheDescriptor>;

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
			, public DownloadCacheDeltaMixins::Enable
			, public DownloadCacheDeltaMixins::Height {
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
				, m_pDownloadEntries(downloadSets.pPrimary)
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		using DownloadCacheDeltaMixins::ConstAccessor::find;
		using DownloadCacheDeltaMixins::MutableAccessor::find;

		bool enabled() const {
			return DownloadCacheEnabled(m_pConfigHolder, height());
		}

	private:
		DownloadCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pDownloadEntries;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
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
