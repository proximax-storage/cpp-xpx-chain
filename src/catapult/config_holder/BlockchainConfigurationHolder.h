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
		using PluginInitializer = std::function<void(model::NetworkConfiguration &)>;

		explicit BlockchainConfigurationHolder(cache::CatapultCache *pCache =  nullptr);

		virtual ~BlockchainConfigurationHolder() = default;

	public:
		/// Extracts the resources path from the command line arguments.
		/// \a argc commmand line arguments are accessible via \a argv.
		static boost::filesystem::path GetResourcesPath(int argc, const char **argv);

		const BlockchainConfiguration &LoadConfig(int argc, const char **argv, const std::string &extensionsHost);

		/// Set \a config at \a height
		void SetConfig(const Height &height, const BlockchainConfiguration &config);

		/// Get \a config at \a height
		virtual const BlockchainConfiguration &Config(const Height &height);

		/// Get latest available config
		virtual const BlockchainConfiguration &Config();

		/// Get config at \a height or latest available config
		virtual const BlockchainConfiguration &ConfigAtHeightOrLatest(const Height &height);

		void SetCache(cache::CatapultCache *pCache) {
			m_pCache = pCache;
		}

		void SetPluginInitializer(const PluginInitializer &initializer) {
			m_pluginInitializer = initializer;
		}

		void InsertConfig(const Height& height, const std::string &strConfig, const std::string &supportedVersion){
			try {
				std::istringstream inputBlock(strConfig);
				auto networkConfig = model::NetworkConfiguration::LoadFromBag(
						utils::ConfigurationBag::FromStream(inputBlock));
				m_pluginInitializer(networkConfig);

				std::istringstream inputVersions(supportedVersion);
				config::SupportedEntityVersions supportedEntityVersions;
				supportedEntityVersions = LoadSupportedEntityVersions(inputVersions);

				const auto &baseConfig = m_networkConfigs.get(Height(0));
				auto config = BlockchainConfiguration(
						baseConfig.Immutable,
						networkConfig,
						baseConfig.Node,
						baseConfig.Logging,
						baseConfig.User,
						baseConfig.Extensions,
						baseConfig.Inflation,
						supportedEntityVersions
				);

				if (m_networkConfigs.containsRef(height) || m_networkConfigs.contains(height))
					m_networkConfigs.erase(height);

				m_networkConfigs.insert(height, config);
			}catch(...) {
				CATAPULT_THROW_INVALID_ARGUMENT_1("unable to insert to config holder at height", height);
			}
		}

		ConfigTreeCache& NetworkConfigs() {
			return m_networkConfigs;
		}

	protected:
		ConfigTreeCache m_networkConfigs;
		cache::CatapultCache* m_pCache;
		PluginInitializer m_pluginInitializer;
		std::mutex m_mutex;
	};
}}
