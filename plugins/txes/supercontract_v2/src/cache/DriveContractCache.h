/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveContractCacheDelta.h"
#include "DriveContractCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

    /// Cache composed of drive contract information.
	using BasicDriveContractCache = BasicCache<DriveContractCacheDescriptor, DriveContractCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of drive contract information.
	class DriveContractCache : public SynchronizedCache<BasicDriveContractCache> {
	public:
		DEFINE_CACHE_CONSTANTS(DriveContract_v2)

	public:
		/// Creates a cache around \a config.
		explicit DriveContractCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SynchronizedCache<BasicDriveContractCache>(BasicDriveContractCache(config, std::move(pConfigHolder)))
		{}
	};
}}