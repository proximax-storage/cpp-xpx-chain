/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NetworkConfigCacheSubCachePlugin.h"

namespace catapult { namespace cache {

	void NetworkConfigCacheSummaryCacheStorage::saveAll(const CatapultCacheView&, io::OutputStream&) const {
		CATAPULT_THROW_INVALID_ARGUMENT("NetworkConfigCacheSummaryCacheStorage does not support saveAll");
	}

	void NetworkConfigCacheSummaryCacheStorage::saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const {
		// write version
		io::Write32(output, 1);

		const auto& delta = cacheDelta.sub<NetworkConfigCache>();
		auto heights = delta.heights();
		io::Write64(output, heights.size());
		for (const auto& height : heights)
			io::Write64(output, height.unwrap());

		output.flush();
	}

	void NetworkConfigCacheSummaryCacheStorage::loadAll(io::InputStream& input, size_t) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of catapult config cache summary", version);

		auto heightsSize = io::Read64(input);
		std::set<Height> heights;
		for (auto i = 0u; i < heightsSize; ++i) {
			heights.insert(Height(io::Read64(input)));
		}
		cache().init(heights);
	}

	NetworkConfigCacheSubCachePlugin::NetworkConfigCacheSubCachePlugin(const CacheConfiguration& config,
	                                                                   const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
			: BaseNetworkConfigCacheSubCachePlugin(std::make_unique<NetworkConfigCache>(config, pConfigHolder))
	{}
}}
