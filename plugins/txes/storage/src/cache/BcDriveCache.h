/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BcDriveCacheDelta.h"
#include "BcDriveCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Cache composed of drive information.
	using BasicBcDriveCache = BasicCache<BcDriveCacheDescriptor, BcDriveCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of drive information.
	class BcDriveCache : public SynchronizedCache<BasicBcDriveCache> {
	public:
		DEFINE_CACHE_CONSTANTS(BcDrive)

	public:
		/// Creates a cache around \a config and \a pConfigHolder.
		explicit BcDriveCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SynchronizedCache<BasicBcDriveCache>(BasicBcDriveCache(config, std::move(pConfigHolder)))
		{}
	};
}}
