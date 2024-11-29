/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataV1CacheDelta.h"
#include "MetadataV1CacheStorage.h"
#include "MetadataV1CacheView.h"
#include "MetadataV1CacheTypes.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	using MetadataV1BasicCache = BasicCache<
		MetadataV1CacheDescriptor,
		MetadataV1CacheTypes::BaseSets>;

	/// Synchronized cache composed of metadata information.
	class MetadataV1Cache : public SynchronizedCache<MetadataV1BasicCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Metadata)

	public:
		/// Creates a cache around \a config and options.
		explicit MetadataV1Cache(const CacheConfiguration& config)
				: SynchronizedCache<MetadataV1BasicCache>(MetadataV1BasicCache(config))
		{}
	};
}}
