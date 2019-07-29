/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultConfigCacheSubCachePlugin.h"

namespace catapult { namespace cache {

	void CatapultConfigCacheSummaryCacheStorage::saveAll(const CatapultCacheView&, io::OutputStream&) const {
		CATAPULT_THROW_INVALID_ARGUMENT("CatapultConfigCacheSummaryCacheStorage does not support saveAll");
	}

	void CatapultConfigCacheSummaryCacheStorage::saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const {
		// write version
		io::Write32(output, 1);

		const auto& delta = cacheDelta.sub<CatapultConfigCache>();
		const auto& heights = delta.heights();
		io::Write64(output, heights.size());
		for (const auto& height : heights)
			io::Write64(output, height.unwrap());

		output.flush();
	}

	void CatapultConfigCacheSummaryCacheStorage::loadAll(io::InputStream& input, size_t) {
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

	CatapultConfigCacheSubCachePlugin::CatapultConfigCacheSubCachePlugin(const CacheConfiguration& config)
			: BaseCatapultConfigCacheSubCachePlugin(std::make_unique<CatapultConfigCache>(config))
	{}
}}
