/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveCacheDelta.h"
#include "DriveCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of drive information.
	using BasicDriveCache = BasicCache<DriveCacheDescriptor, DriveCacheTypes::BaseSets>;

	/// Synchronized cache composed of drive information.
	class DriveCache : public SynchronizedCache<BasicDriveCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Drive)

	public:
		/// Creates a cache around \a config.
		explicit DriveCache(const CacheConfiguration& config) : SynchronizedCache<BasicDriveCache>(BasicDriveCache(config))
		{}
	};
}}
