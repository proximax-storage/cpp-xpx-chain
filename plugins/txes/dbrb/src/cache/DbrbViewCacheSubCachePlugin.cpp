/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbViewCacheSubCachePlugin.h"

namespace catapult { namespace cache {

	void DbrbViewCacheSummaryCacheStorage::saveAll(const CatapultCacheView&, io::OutputStream&) const {
		CATAPULT_THROW_INVALID_ARGUMENT("DbrbViewCacheSummaryCacheStorage does not support saveAll");
	}

	void DbrbViewCacheSummaryCacheStorage::saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const {
		// write version
		io::Write32(output, 1);

		const auto& delta = cacheDelta.sub<DbrbViewCache>();
		auto processIds = delta.processIds();
		io::Write64(output, processIds.size());
		for (const auto& processId : processIds)
			io::Write(output, processId);

		output.flush();
	}

	void DbrbViewCacheSummaryCacheStorage::loadAll(io::InputStream& input, size_t) {
		std::set<dbrb::ProcessId> processIds;
		if (!input.eof()) {
			VersionType version = io::Read32(input);
			if (version > 1)
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of DBRB view cache summary", version);

			auto count = io::Read64(input);
			for (auto i = 0u; i < count; ++i) {
				dbrb::ProcessId processId;
				io::Read(input, processId);
				processIds.insert(processId);
			}
		}

		cache().init(processIds);
	}

	DbrbViewCacheSubCachePlugin::DbrbViewCacheSubCachePlugin(
			const CacheConfiguration& config,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			std::shared_ptr<DbrbViewFetcherImpl> pDbrbViewFetcher)
		: BaseDbrbViewCacheSubCachePlugin(std::make_unique<DbrbViewCache>(config, pConfigHolder, pDbrbViewFetcher))
	{}
}}
