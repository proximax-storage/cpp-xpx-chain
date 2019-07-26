/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config/CatapultConfiguration.h"

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace config {

	constexpr Height HEIGHT_OF_LATEST_CONFIG = Height(-1);

	class LocalNodeConfigurationHolder {
	public:
		explicit LocalNodeConfigurationHolder(cache::CatapultCache* pCache);
		virtual ~LocalNodeConfigurationHolder() = default;

	public:
		/// Extracts the resources path from the command line arguments.
		/// \a argc commmand line arguments are accessible via \a argv.
		static boost::filesystem::path GetResourcesPath(int argc, const char** argv);
		const CatapultConfiguration& LoadConfig(int argc, const char** argv, const std::string& extensionsHost);

		/// Set \a config at \a height
		void SetConfig(const Height& height, const CatapultConfiguration& config);

		/// Get \a config at \a height
		virtual CatapultConfiguration& Config(const Height& height);

		/// Get latest available config
		virtual CatapultConfiguration& Config();

		/// Get config at \a height or latest available config
		virtual CatapultConfiguration& ConfigAtHeightOrLatest(const Height& height);

		void SetCache(cache::CatapultCache* pCache) {
			m_pCache = pCache;
		}

	protected:
		std::map<Height, std::shared_ptr<CatapultConfiguration>> m_catapultConfigs;
		cache::CatapultCache* m_pCache;
	};
}}
