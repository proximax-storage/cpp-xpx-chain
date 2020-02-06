/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadCacheDelta.h"
#include "DownloadCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Cache composed of download information.
	using BasicDownloadCache = BasicCache<DownloadCacheDescriptor, DownloadCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of download information.
	class DownloadCache : public SynchronizedCache<BasicDownloadCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Download)

	public:
		/// Creates a cache around \a config and \a pConfigHolder.
		explicit DownloadCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SynchronizedCache<BasicDownloadCache>(BasicDownloadCache(config, std::move(pConfigHolder)))
		{}
	};
}}
