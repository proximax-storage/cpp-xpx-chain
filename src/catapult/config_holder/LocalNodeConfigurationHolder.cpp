/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LocalNodeConfigurationHolder.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/exceptions.h"
#include "catapult/io/Stream.h"
#include "plugins/txes/config/src/cache/CatapultConfigCache.h"
#include <fstream>

namespace catapult { namespace config {

	LocalNodeConfigurationHolder::LocalNodeConfigurationHolder(cache::CatapultCache* pCache)
			: m_pCache(pCache) {
		auto config = CatapultConfiguration{
			model::BlockChainConfiguration::Uninitialized(),
			NodeConfiguration::Uninitialized(),
			LoggingConfiguration::Uninitialized(),
			UserConfiguration::Uninitialized(),
			ExtensionsConfiguration::Uninitialized(),
			InflationConfiguration::Uninitialized(),
			SupportedEntityVersions()
		};

		SetConfig(Height(0), config);
	}

	boost::filesystem::path LocalNodeConfigurationHolder::GetResourcesPath(int argc, const char** argv) {
		return boost::filesystem::path(argc > 1 ? argv[1] : "..") / "resources";
	}

	const CatapultConfiguration& LocalNodeConfigurationHolder::LoadConfig(int argc, const char** argv, const std::string& extensionsHost) {
		auto resourcesPath = GetResourcesPath(argc, argv);
		std::cout << "loading resources from " << resourcesPath << std::endl;
		SetConfig(Height(0), config::CatapultConfiguration::LoadFromPath(resourcesPath, extensionsHost));
		return m_catapultConfigs.get(Height(0));
	}

	void LocalNodeConfigurationHolder::SetConfig(const Height& height, const CatapultConfiguration& config) {
		std::lock_guard<std::mutex> guard(m_mutex);
		if (m_catapultConfigs.contains(height))
			m_catapultConfigs.erase(height);
		m_catapultConfigs.insert(height, config);
	}

	CatapultConfiguration& LocalNodeConfigurationHolder::Config(const Height& height) {
		std::lock_guard<std::mutex> guard(m_mutex);
		if (m_catapultConfigs.contains(height))
			return m_catapultConfigs.get(height);

		// While we are loading nemesis block, config from cache is not ready, so let's use initial config
		if (height.unwrap() == 1)
			return m_catapultConfigs.get(Height(0));

		if (!m_pCache)
			CATAPULT_THROW_INVALID_ARGUMENT("cache pointer is not set");

		auto configCache = m_pCache->sub<cache::CatapultConfigCache>().createView(m_pCache->height());
		auto configHeight = configCache->FindConfigHeightAt(height);

		if (m_catapultConfigs.contains(configHeight))
			return m_catapultConfigs.insertRef(height, configHeight);

		if (configHeight.unwrap() == 0)
			CATAPULT_THROW_INVALID_ARGUMENT_1("failed to find config at height ", height);

		auto entry = configCache->find(configHeight).get();

		std::istringstream inputBlock(entry.blockChainConfig());
		auto blockChainConfig =  model::BlockChainConfiguration::LoadFromBag(utils::ConfigurationBag::FromStream(inputBlock));

		std::istringstream inputVersions(entry.supportedEntityVersions());
		config::SupportedEntityVersions supportedEntityVersions;
		supportedEntityVersions = LoadSupportedEntityVersions(inputVersions);

		const auto& baseConfig = m_catapultConfigs.get(Height(0));
		auto config = CatapultConfiguration(
			blockChainConfig,
			baseConfig.Node,
			baseConfig.Logging,
			baseConfig.User,
			baseConfig.Extensions,
			baseConfig.Inflation,
			supportedEntityVersions
		);

		auto& configRef = m_catapultConfigs.insert(configHeight, config);

		if (configHeight != height)
			m_catapultConfigs.insertRef(height, configHeight);

		return configRef;
	}

	CatapultConfiguration& LocalNodeConfigurationHolder::Config() {
		return Config(m_pCache != NULL ? m_pCache->height() : Height(0));
	}

	CatapultConfiguration& LocalNodeConfigurationHolder::ConfigAtHeightOrLatest(const Height& height) {
		if (height == HEIGHT_OF_LATEST_CONFIG)
			return Config();
		return Config(height);
	}
}}
