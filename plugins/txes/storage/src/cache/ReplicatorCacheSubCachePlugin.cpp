/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorCacheSubCachePlugin.h"

namespace catapult { namespace cache {

	void ReplicatorCacheSummaryCacheStorage::saveAll(const CatapultCacheView&, io::OutputStream&) const {
		CATAPULT_THROW_INVALID_ARGUMENT("ReplicatorCacheSummaryCacheStorage does not support saveAll");
	}

	void ReplicatorCacheSummaryCacheStorage::saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const {
		// write version
		io::Write32(output, 1);

		const auto& delta = cacheDelta.sub<ReplicatorCache>();
		const auto& keys = delta.keys();
		io::Write64(output, keys.size());
		for (const auto& key : keys)
			io::Write(output, key);

		output.flush();
	}

	void ReplicatorCacheSummaryCacheStorage::loadAll(io::InputStream& input, size_t) {
		std::unordered_set<Key, utils::ArrayHasher<Key>> keys;
		// TODO: remove this temporary workaround after mainnet upgrade to 0.8.0
		if (!input.eof()) {
			// read version
			VersionType version = io::Read32(input);
			if (version > 1)
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of replicator cache summary", version);

			auto keysSize = io::Read64(input);
			for (auto i = 0u; i < keysSize; ++i) {
				Key key;
				io::Read(input, key);
				keys.insert(key);
			}
		}

		cache().init(keys);
	}

	ReplicatorCacheSubCachePlugin::ReplicatorCacheSubCachePlugin(
		const CacheConfiguration& config,
		const std::shared_ptr<ReplicatorKeyCollector>& pKeyCollector,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
			: BaseReplicatorCacheSubCachePlugin(std::make_unique<ReplicatorCache>(config, pKeyCollector, pConfigHolder))
	{}
}}
