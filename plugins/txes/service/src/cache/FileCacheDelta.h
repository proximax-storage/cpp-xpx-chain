/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "FileBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
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
		/// Creates a delta around \a fileSets.
		explicit BasicFileCacheDelta(const FileCacheTypes::BaseSetDeltaPointers& fileSets)
				: FileCacheDeltaMixins::Size(*fileSets.pPrimary)
				, FileCacheDeltaMixins::Contains(*fileSets.pPrimary)
				, FileCacheDeltaMixins::ConstAccessor(*fileSets.pPrimary)
				, FileCacheDeltaMixins::MutableAccessor(*fileSets.pPrimary)
				, FileCacheDeltaMixins::PatriciaTreeDelta(*fileSets.pPrimary, fileSets.pPatriciaTree)
				, FileCacheDeltaMixins::BasicInsertRemove(*fileSets.pPrimary)
				, FileCacheDeltaMixins::DeltaElements(*fileSets.pPrimary)
				, m_pFileEntries(fileSets.pPrimary)
		{}

	public:
		using FileCacheDeltaMixins::ConstAccessor::find;
		using FileCacheDeltaMixins::MutableAccessor::find;

	private:
		FileCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pFileEntries;
	};

	/// Delta on top of the file cache.
	class FileCacheDelta : public ReadOnlyViewSupplier<BasicFileCacheDelta> {
	public:
		/// Creates a delta around \a fileSets.
		explicit FileCacheDelta(const FileCacheTypes::BaseSetDeltaPointers& fileSets)
				: ReadOnlyViewSupplier(fileSets)
		{}
	};
}}
