/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadChannelCacheDelta.h"
#include "DownloadChannelCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Cache composed of download information.
	using BasicDownloadChannelCache = BasicCache<DownloadChannelCacheDescriptor, DownloadChannelCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of download information.
	class DownloadChannelCache : public SynchronizedCache<BasicDownloadChannelCache> {
	public:
		DEFINE_CACHE_CONSTANTS(DownloadChannel)

	public:
		/// Creates a cache around \a config and \a pConfigHolder.
		explicit DownloadChannelCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SynchronizedCache<BasicDownloadChannelCache>(BasicDownloadChannelCache(config, std::move(pConfigHolder)))
		{}
	};
}}
