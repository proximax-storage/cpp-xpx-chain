/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LiquidityProviderCacheSubCachePlugin.h"

namespace catapult { namespace cache {

	void LiquidityProviderCacheSummaryCacheStorage::saveAll(const CatapultCacheView&, io::OutputStream&) const {
		CATAPULT_THROW_INVALID_ARGUMENT("LiquidityProviderCacheSummaryCacheStorage does not support saveAll");
	}

	void LiquidityProviderCacheSummaryCacheStorage::saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const {
		// write version
		io::Write32(output, 1);

		const auto& delta = cacheDelta.sub<LiquidityProviderCache>();
		const auto& keys = delta.keys();
		io::Write64(output, keys.size());
		for (const auto& key : keys)
			io::Write(output, key);

		output.flush();
	}

	void LiquidityProviderCacheSummaryCacheStorage::loadAll(io::InputStream& input, size_t) {
		std::unordered_set<UnresolvedMosaicId, utils::BaseValueHasher<UnresolvedMosaicId>> keys;
		// TODO: remove this temporary workaround after mainnet upgrade to 0.8.0
		if (!input.eof()) {
			// read version
			VersionType version = io::Read32(input);
			if (version > 1)
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of LiquidityProvider cache summary", version);

			auto keysSize = io::Read64(input);
			for (auto i = 0u; i < keysSize; ++i) {
				auto key = UnresolvedMosaicId(io::Read64(input));
				keys.insert(key);
			}
		}

		cache().init(keys);
	}

	LiquidityProviderCacheSubCachePlugin::LiquidityProviderCacheSubCachePlugin(
		const CacheConfiguration& config,
		const std::shared_ptr<LiquidityProviderKeyCollector>& pKeyCollector,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
			: BaseLiquidityProviderCacheSubCachePlugin(std::make_unique<LiquidityProviderCache>(config, pKeyCollector, pConfigHolder))
	{}
}}
