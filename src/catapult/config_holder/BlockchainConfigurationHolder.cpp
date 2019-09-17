/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlockchainConfigurationHolder.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/exceptions.h"
#include "catapult/io/Stream.h"
#include "plugins/txes/config/src/cache/NetworkConfigCache.h"
#include <fstream>

namespace catapult { namespace config {

	BlockchainConfigurationHolder::BlockchainConfigurationHolder(cache::CatapultCache* pCache)
			: m_pCache(pCache) {
		auto config = BlockchainConfiguration{
			ImmutableConfiguration::Uninitialized(),
			model::NetworkConfiguration::Uninitialized(),
			NodeConfiguration::Uninitialized(),
			LoggingConfiguration::Uninitialized(),
			UserConfiguration::Uninitialized(),
			ExtensionsConfiguration::Uninitialized(),
			InflationConfiguration::Uninitialized(),
			SupportedEntityVersions()
		};

		SetConfig(Height(0), config);
	}

	boost::filesystem::path BlockchainConfigurationHolder::GetResourcesPath(int argc, const char** argv) {
		return boost::filesystem::path(argc > 1 ? argv[1] : "..") / "resources";
	}

	const BlockchainConfiguration& BlockchainConfigurationHolder::LoadConfig(int argc, const char** argv, const std::string& extensionsHost) {
		auto resourcesPath = GetResourcesPath(argc, argv);
		std::cout << "loading resources from " << resourcesPath << std::endl;
		SetConfig(Height(0), config::BlockchainConfiguration::LoadFromPath(resourcesPath, extensionsHost));
		return m_networkConfigs.get(Height(0));
	}

	void BlockchainConfigurationHolder::SetConfig(const Height& height, const BlockchainConfiguration& config) {
		std::lock_guard<std::mutex> guard(m_mutex);
		if (m_networkConfigs.contains(height))
			m_networkConfigs.erase(height);
		m_networkConfigs.insert(height, config);
	}

	BlockchainConfiguration& BlockchainConfigurationHolder::Config(const Height& height) {
		std::lock_guard<std::mutex> guard(m_mutex);
		if (m_networkConfigs.contains(height))
			return m_networkConfigs.get(height);

		// While we are loading nemesis block, config from cache is not ready, so let's use initial config
		if (height.unwrap() == 1)
			return m_networkConfigs.get(Height(0));

		if (!m_pCache)
			CATAPULT_THROW_INVALID_ARGUMENT("cache pointer is not set");

		auto configCache = m_pCache->sub<cache::NetworkConfigCache>().createView(height);
		auto configHeight = configCache->FindConfigHeightAt(height);

		if (m_networkConfigs.contains(configHeight))
			return m_networkConfigs.insertRef(height, configHeight);

		if (configHeight.unwrap() == 0)
			CATAPULT_THROW_INVALID_ARGUMENT_1("failed to find config at height ", height);

		auto entry = configCache->find(configHeight).get();

		std::istringstream inputBlock(entry.networkConfig());
		auto networkConfig =  model::NetworkConfiguration::LoadFromBag(utils::ConfigurationBag::FromStream(inputBlock));

		std::istringstream inputVersions(entry.supportedEntityVersions());
		config::SupportedEntityVersions supportedEntityVersions;
		supportedEntityVersions = LoadSupportedEntityVersions(inputVersions);

		const auto& baseConfig = m_networkConfigs.get(Height(0));
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

		auto& configRef = m_networkConfigs.insert(configHeight, config);

		if (configHeight != height)
			m_networkConfigs.insertRef(height, configHeight);

		return configRef;
	}

	BlockchainConfiguration& BlockchainConfigurationHolder::Config() {
		return Config(m_pCache != nullptr ? m_pCache->configHeight() : Height(0));
	}

	BlockchainConfiguration& BlockchainConfigurationHolder::ConfigAtHeightOrLatest(const Height& height) {
		if (height == HEIGHT_OF_LATEST_CONFIG)
			return Config();
		return Config(height);
	}
}}
