/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlockchainUpgradeCacheSubCachePlugin.h"

namespace catapult { namespace cache {

	void BlockchainUpgradeCacheSummaryCacheStorage::saveAll(const CatapultCacheView&, io::OutputStream&) const {
		CATAPULT_THROW_INVALID_ARGUMENT("BlockchainUpgradeCacheSummaryCacheStorage does not support saveAll");
	}

	void BlockchainUpgradeCacheSummaryCacheStorage::saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const {
	}

	void BlockchainUpgradeCacheSummaryCacheStorage::loadAll(io::InputStream& input, size_t) {
		cache().init();
	}

	BlockchainUpgradeCacheSubCachePlugin::BlockchainUpgradeCacheSubCachePlugin(
		const CacheConfiguration& config,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
			: BaseBlockchainUpgradeCacheSubCachePlugin(std::make_unique<BlockchainUpgradeCache>(config, pConfigHolder))
	{}
}}
