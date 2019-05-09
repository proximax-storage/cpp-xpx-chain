/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataCacheDelta.h"
#include "MetadataCacheStorage.h"
#include "MetadataCacheView.h"
#include "MetadataCacheTypes.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	using MetadataBasicCache = BasicCache<
		MetadataCacheDescriptor,
		MetadataCacheTypes::BaseSets>;

	/// Synchronized cache composed of metadata information.
	class MetadataCache : public SynchronizedCache<MetadataBasicCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Metadata)

	public:
		/// Creates a cache around \a config and options.
		explicit MetadataCache(const CacheConfiguration& config)
				: SynchronizedCache<MetadataBasicCache>(MetadataBasicCache(config))
		{}
	};
}}
