/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config/CatapultConfiguration.h"

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace config {

	class LocalNodeConfigurationHolder {
	public:
		LocalNodeConfigurationHolder(cache::CatapultCache* pCache);

	public:
		/// Extracts the resources path from the command line arguments.
		/// \a argc commmand line arguments are accessible via \a argv.
		static boost::filesystem::path GetResourcesPath(int argc, const char** argv);
		const CatapultConfiguration& LoadConfig(int argc, const char** argv);

		void SetConfig(const Height& height, const CatapultConfiguration& config);
		virtual CatapultConfiguration& Config(const Height& height);

		void SetCache(cache::CatapultCache* pCache) {
			m_pCache = pCache;
		}

	protected:
		std::map<Height, CatapultConfiguration> m_catapultConfigs;
		cache::CatapultCache* m_pCache;
	};
}}
