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

	LocalNodeConfigurationHolder::LocalNodeConfigurationHolder(cache::CatapultCache* pCache)
			: m_pCache(pCache) {
		auto config = LocalNodeConfiguration{
			model::BlockChainConfiguration::Uninitialized(),
			NodeConfiguration::Uninitialized(),
			LoggingConfiguration::Uninitialized(),
			UserConfiguration::Uninitialized(),
			SupportedEntityVersions()};
		m_catapultConfigs.insert({ Height{0}, config });
	}

	boost::filesystem::path LocalNodeConfigurationHolder::GetResourcesPath(int argc, const char** argv) {
		return boost::filesystem::path(argc > 1 ? argv[1] : "..") / "resources";
	}

	const LocalNodeConfiguration& LocalNodeConfigurationHolder::LoadConfig(int argc, const char** argv) {
		auto resourcesPath = GetResourcesPath(argc, argv);
		std::cout << "loading resources from " << resourcesPath << std::endl;
		SetConfig(Height{0}, config::LocalNodeConfiguration::LoadFromPath(resourcesPath));

		return m_catapultConfigs.at(Height{0});
	}

	void LocalNodeConfigurationHolder::SetConfig(const Height& height, const LocalNodeConfiguration& config) {
		if (m_catapultConfigs.count(height))
			m_catapultConfigs.erase(height);
		m_catapultConfigs.insert({ height, config });
	}

	LocalNodeConfiguration& LocalNodeConfigurationHolder::Config(const Height& height) {
		if (m_catapultConfigs.count(height))
			return m_catapultConfigs.at(height);

		if (!m_pCache)
			CATAPULT_THROW_INVALID_ARGUMENT("cache pointer is not set");

		auto& configCache = m_pCache->createView().sub<cache::CatapultConfigCache>();
		auto configHeight = configCache.FindConfigHeightAt(height);
		if (m_catapultConfigs.count(configHeight))
			return m_catapultConfigs.at(configHeight);

		auto iter = std::lower_bound(m_catapultConfigs.begin(), m_catapultConfigs.end(), configHeight,
			[](const auto& pair, const auto& height) { return pair.first < height; });
		LocalNodeConfiguration config((iter--)->second);

		auto entry = configCache.find(configHeight).get();

		if (entry.blockChainConfig().empty()) {
			const_cast<model::BlockChainConfiguration&>(config.BlockChain) = Config(configHeight - Height{1}).BlockChain;
		} else {
			std::istringstream input(entry.blockChainConfig());
			auto bag = utils::ConfigurationBag::FromStream(input);
			const_cast<model::BlockChainConfiguration&>(config.BlockChain) = model::BlockChainConfiguration::LoadFromBag(bag);
		}

		if (entry.supportedEntityVersions().empty()) {
			const_cast<config::SupportedEntityVersions&>(config.SupportedEntityVersions) = Config(configHeight - Height{1}).SupportedEntityVersions;
		} else {
			std::istringstream input(entry.supportedEntityVersions());
			const_cast<config::SupportedEntityVersions&>(config.SupportedEntityVersions) = LoadSupportedEntityVersions(input);
		}

		m_catapultConfigs.insert({ configHeight, config });

		return m_catapultConfigs.at(configHeight);
	}
}}
