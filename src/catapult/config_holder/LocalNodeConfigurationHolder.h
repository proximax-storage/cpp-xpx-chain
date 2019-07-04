/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config/LocalNodeConfiguration.h"

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace config {

	class LocalNodeConfigurationHolder {
	public:
		LocalNodeConfigurationHolder(cache::CatapultCache* pCache = nullptr);

	public:
		/// Extracts the resources path from the command line arguments.
		/// \a argc commmand line arguments are accessible via \a argv.
		static boost::filesystem::path GetResourcesPath(int argc, const char** argv);
		const LocalNodeConfiguration& LoadConfig(int argc, const char** argv);

		LocalNodeConfiguration& Config(const Height& height);

		void SetConfig(const Height& height, const LocalNodeConfiguration& config);
		void SetBlockChainConfig(const Height&, const model::BlockChainConfiguration&);

		void SetCache(cache::CatapultCache* pCache);

	private:
		std::map<Height, LocalNodeConfiguration> m_catapultConfigs;
		cache::CatapultCache* m_pCache;
	};
}}
