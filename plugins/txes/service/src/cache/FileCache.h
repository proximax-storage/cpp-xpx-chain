/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "FileCacheDelta.h"
#include "FileCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of file information.
	using BasicFileCache = BasicCache<FileCacheDescriptor, FileCacheTypes::BaseSets>;

	/// Synchronized cache composed of file information.
	class FileCache : public SynchronizedCache<BasicFileCache> {
	public:
		DEFINE_CACHE_CONSTANTS(File)

	public:
		/// Creates a cache around \a config.
		explicit FileCache(const CacheConfiguration& config) : SynchronizedCache<BasicFileCache>(BasicFileCache(config))
		{}
	};
}}
