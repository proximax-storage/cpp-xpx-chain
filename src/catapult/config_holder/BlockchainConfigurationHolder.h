/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config/BlockchainConfiguration.h"
#include "ConfigTreeCache.h"
#include <mutex>

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace config {

	constexpr Height HEIGHT_OF_LATEST_CONFIG = Height(-1);

	class BlockchainConfigurationHolder {
	public:
		explicit BlockchainConfigurationHolder(cache::CatapultCache* pCache);
		virtual ~BlockchainConfigurationHolder() = default;

	public:
		/// Extracts the resources path from the command line arguments.
		/// \a argc commmand line arguments are accessible via \a argv.
		static boost::filesystem::path GetResourcesPath(int argc, const char** argv);

		const BlockchainConfiguration& LoadConfig(int argc, const char** argv, const std::string& extensionsHost);

		/// Set \a config at \a height
		void SetConfig(const Height& height, const BlockchainConfiguration& config);

		/// Get \a config at \a height
		virtual BlockchainConfiguration& Config(const Height& height);

		/// Get latest available config
		virtual BlockchainConfiguration& Config();

		/// Get config at \a height or latest available config
		virtual BlockchainConfiguration& ConfigAtHeightOrLatest(const Height& height);

		void SetCache(cache::CatapultCache* pCache) {
			m_pCache = pCache;
		}

	protected:
		ConfigTreeCache m_networkConfigs;
		cache::CatapultCache* m_pCache;
		std::mutex m_mutex;
	};
}}
