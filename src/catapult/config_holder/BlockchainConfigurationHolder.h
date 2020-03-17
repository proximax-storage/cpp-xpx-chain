/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config/BlockchainConfiguration.h"
#include <mutex>

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace config {

	constexpr Height HEIGHT_OF_LATEST_CONFIG = Height(-1);

	class BlockchainConfigurationHolder {
	public:
		using PluginInitializer = std::function<void(model::NetworkConfiguration&)>;

		explicit BlockchainConfigurationHolder(cache::CatapultCache* pCache =  nullptr);
		explicit BlockchainConfigurationHolder(const BlockchainConfiguration& config);

		virtual ~BlockchainConfigurationHolder() = default;

	public:
		/// Extracts the resources path from the command line arguments.
		/// \a argc commmand line arguments are accessible via \a argv.
		static boost::filesystem::path GetResourcesPath(int argc, const char** argv);

		/// Get \a config at \a height
		virtual const BlockchainConfiguration& Config(const Height& height);

		/// Get latest available config
		virtual const BlockchainConfiguration& Config();

		/// Get config at \a height or latest available config
		virtual const BlockchainConfiguration& ConfigAtHeightOrLatest(const Height& height);

		void SetCache(cache::CatapultCache* pCache) {
			m_pCache = pCache;
		}

		void SetPluginInitializer(const PluginInitializer& initializer) {
			m_pluginInitializer = initializer;
		}

		void InsertConfig(const Height& height, const std::string& strConfig, const std::string& supportedVersion);
		void RemoveConfig(const Height& height);

	protected:
		std::map<Height, BlockchainConfiguration> m_configs;
		cache::CatapultCache* m_pCache;
		PluginInitializer m_pluginInitializer;
		std::mutex m_mutex;
	};
}}
