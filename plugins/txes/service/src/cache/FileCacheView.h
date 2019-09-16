/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "FileBaseSets.h"
#include "FileCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

	/// Mixins used by the file cache view.
	using FileCacheViewMixins = PatriciaTreeCacheMixins<FileCacheTypes::PrimaryTypes::BaseSetType, FileCacheDescriptor>;

	/// Basic view on top of the file cache.
	class BasicFileCacheView
			: public utils::MoveOnly
			, public FileCacheViewMixins::Size
			, public FileCacheViewMixins::Contains
			, public FileCacheViewMixins::Iteration
			, public FileCacheViewMixins::ConstAccessor
			, public FileCacheViewMixins::PatriciaTreeView
			, public FileCacheViewMixins::Enable
			, public FileCacheViewMixins::Height {
	public:
		using ReadOnlyView = FileCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a view around \a fileSets.
		explicit BasicFileCacheView(const FileCacheTypes::BaseSets& fileSets)
				: FileCacheViewMixins::Size(fileSets.Primary)
				, FileCacheViewMixins::Contains(fileSets.Primary)
				, FileCacheViewMixins::Iteration(fileSets.Primary)
				, FileCacheViewMixins::ConstAccessor(fileSets.Primary)
				, FileCacheViewMixins::PatriciaTreeView(fileSets.PatriciaTree.get())
		{}
	};

	/// View on top of the file cache.
	class FileCacheView : public ReadOnlyViewSupplier<BasicFileCacheView> {
	public:
		/// Creates a view around \a fileSets.
		explicit FileCacheView(const FileCacheTypes::BaseSets& fileSets)
				: ReadOnlyViewSupplier(fileSets)
		{}
	};
}}
