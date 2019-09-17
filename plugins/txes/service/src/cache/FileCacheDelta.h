/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "FileBaseSets.h"
#include "ServiceCacheTools.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the file cache delta.
	using FileCacheDeltaMixins = PatriciaTreeCacheMixins<FileCacheTypes::PrimaryTypes::BaseSetDeltaType, FileCacheDescriptor>;

	/// Basic delta on top of the file cache.
	class BasicFileCacheDelta
			: public utils::MoveOnly
			, public FileCacheDeltaMixins::Size
			, public FileCacheDeltaMixins::Contains
			, public FileCacheDeltaMixins::ConstAccessor
			, public FileCacheDeltaMixins::MutableAccessor
			, public FileCacheDeltaMixins::PatriciaTreeDelta
			, public FileCacheDeltaMixins::BasicInsertRemove
			, public FileCacheDeltaMixins::DeltaElements
			, public FileCacheDeltaMixins::Enable
			, public FileCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = FileCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a fileSets and \a pConfigHolder.
		explicit BasicFileCacheDelta(
			const FileCacheTypes::BaseSetDeltaPointers& fileSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: FileCacheDeltaMixins::Size(*fileSets.pPrimary)
				, FileCacheDeltaMixins::Contains(*fileSets.pPrimary)
				, FileCacheDeltaMixins::ConstAccessor(*fileSets.pPrimary)
				, FileCacheDeltaMixins::MutableAccessor(*fileSets.pPrimary)
				, FileCacheDeltaMixins::PatriciaTreeDelta(*fileSets.pPrimary, fileSets.pPatriciaTree)
				, FileCacheDeltaMixins::BasicInsertRemove(*fileSets.pPrimary)
				, FileCacheDeltaMixins::DeltaElements(*fileSets.pPrimary)
				, m_pFileEntries(fileSets.pPrimary)
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		using FileCacheDeltaMixins::ConstAccessor::find;
		using FileCacheDeltaMixins::MutableAccessor::find;

		bool enabled() const {
			return ServicePluginEnabled(m_pConfigHolder, height());
		}

	private:
		FileCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pFileEntries;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// Delta on top of the file cache.
	class FileCacheDelta : public ReadOnlyViewSupplier<BasicFileCacheDelta> {
	public:
		/// Creates a delta around \a fileSets and \a pConfigHolder.
		explicit FileCacheDelta(
			const FileCacheTypes::BaseSetDeltaPointers& fileSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: ReadOnlyViewSupplier(fileSets, pConfigHolder)
		{}
	};
}}
