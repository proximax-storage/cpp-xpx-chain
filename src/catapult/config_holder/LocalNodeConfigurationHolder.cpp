/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LocalNodeConfigurationHolder.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/exceptions.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/io/Stream.h"
#include "plugins/txes/config/src/cache/CatapultConfigCache.h"
#include <fstream>

namespace catapult { namespace config {

	namespace {
		CatapultConfiguration& insertConfigAtHeight(std::map<Height, CatapultConfiguration>& configs, const Height& height, CatapultConfiguration config) {
			while (configs.size() > configs.begin()->second.BlockChain.MaxRollbackBlocks) {
				configs.erase(configs.begin());
			}
			configs.insert({ height, config });

			return configs.at(height);
		}
	}

	LocalNodeConfigurationHolder::LocalNodeConfigurationHolder(cache::CatapultCache* pCache)
			: m_pCache(pCache) {
		auto config = CatapultConfiguration{
			model::BlockChainConfiguration::Uninitialized(),
			NodeConfiguration::Uninitialized(),
			LoggingConfiguration::Uninitialized(),
			UserConfiguration::Uninitialized(),
			ExtensionsConfiguration::Uninitialized(),
			InflationConfiguration::Uninitialized(),
			SupportedEntityVersions()};
		insertConfigAtHeight(m_catapultConfigs, Height{0}, config);
	}

	boost::filesystem::path LocalNodeConfigurationHolder::GetResourcesPath(int argc, const char** argv) {
		return boost::filesystem::path(argc > 1 ? argv[1] : "..") / "resources";
	}

	const CatapultConfiguration& LocalNodeConfigurationHolder::LoadConfig(int argc, const char** argv, const std::string& extensionsHost) {
		auto resourcesPath = GetResourcesPath(argc, argv);
		std::cout << "loading resources from " << resourcesPath << std::endl;
		SetConfig(Height{0}, config::CatapultConfiguration::LoadFromPath(resourcesPath, extensionsHost));

		return m_catapultConfigs.at(Height{0});
	}

	void LocalNodeConfigurationHolder::SetConfig(const Height& height, const CatapultConfiguration& config) {
		if (m_catapultConfigs.count(height))
			m_catapultConfigs.erase(height);
		insertConfigAtHeight(m_catapultConfigs, height, config);
	}

	CatapultConfiguration& LocalNodeConfigurationHolder::Config(const Height& height) {
		if (m_catapultConfigs.count(height))
			return m_catapultConfigs.at(height);

		if (!m_pCache)
			CATAPULT_THROW_INVALID_ARGUMENT("cache pointer is not set");

		auto configCache = m_pCache->sub<cache::CatapultConfigCache>().createView(m_pCache->height());
		auto configHeight = configCache->FindConfigHeightAt(height);

		if (m_catapultConfigs.count(configHeight)) {
			return insertConfigAtHeight(m_catapultConfigs, height, m_catapultConfigs.at(configHeight));
		}

		if (configHeight.unwrap() == 0)
			CATAPULT_THROW_INVALID_ARGUMENT_1("didn't find available config at height ", height);

		auto entry = configCache->find(configHeight).get();

		auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
		if (entry.blockChainConfig().empty()) {
			blockChainConfig = Config(configHeight - Height{1}).BlockChain;
		} else {
			std::istringstream input(entry.blockChainConfig());
			blockChainConfig = model::BlockChainConfiguration::LoadFromBag(utils::ConfigurationBag::FromStream(input));
		}

		config::SupportedEntityVersions supportedEntityVersions;
		if (entry.supportedEntityVersions().empty()) {
			supportedEntityVersions = Config(configHeight - Height{1}).SupportedEntityVersions;
		} else {
			std::istringstream input(entry.supportedEntityVersions());
			supportedEntityVersions = LoadSupportedEntityVersions(input);
		}

		auto iter = m_catapultConfigs.begin();
		auto config = CatapultConfiguration(
			blockChainConfig,
			iter->second.Node,
			iter->second.Logging,
			iter->second.User,
			iter->second.Extensions,
			iter->second.Inflation,
			supportedEntityVersions
		);

		return insertConfigAtHeight(m_catapultConfigs, height, config);
	}

	CatapultConfiguration& LocalNodeConfigurationHolder::Config() {
		return Config(m_pCache != NULL ? m_pCache->height() : Height(0));
	}

	CatapultConfiguration& LocalNodeConfigurationHolder::ConfigAtHeightOrDefault(const Height& height) {
		if (height.unwrap() == 0)
			return Config();
		return Config(height);
	}
}}
