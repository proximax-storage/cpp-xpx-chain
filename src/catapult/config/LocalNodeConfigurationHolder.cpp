/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LocalNodeConfigurationHolder.h"
#include "catapult/exceptions.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/io/Stream.h"
#include <fstream>

namespace catapult { namespace config {

	LocalNodeConfigurationHolder::LocalNodeConfigurationHolder()
		: m_currentLocalNodeConfig(
			model::BlockChainConfiguration::Uninitialized(),
			NodeConfiguration::Uninitialized(),
			LoggingConfiguration::Uninitialized(),
			UserConfiguration::Uninitialized())
		, m_pDelta(nullptr)
	{}

	boost::filesystem::path LocalNodeConfigurationHolder::GetResourcesPath(int argc, const char** argv) {
		return boost::filesystem::path(argc > 1 ? argv[1] : "..") / "resources";
	}

	const LocalNodeConfiguration& LocalNodeConfigurationHolder::Config() const {
		return m_currentLocalNodeConfig;
	}

	void LocalNodeConfigurationHolder::SetConfig(const LocalNodeConfiguration& config) {
		const_cast<model::BlockChainConfiguration&>(m_currentLocalNodeConfig.BlockChain) = config.BlockChain;
		const_cast<NodeConfiguration&>(m_currentLocalNodeConfig.Node) = config.Node;
		const_cast<LoggingConfiguration&>(m_currentLocalNodeConfig.Logging) = config.Logging;
		const_cast<UserConfiguration&>(m_currentLocalNodeConfig.User) = config.User;
	}

	const LocalNodeConfiguration& LocalNodeConfigurationHolder::LoadConfig(int argc, const char** argv) {
		auto resourcesPath = GetResourcesPath(argc, argv);
		std::cout << "loading resources from " << resourcesPath << std::endl;
		SetConfig(config::LocalNodeConfiguration::LoadFromPath(resourcesPath));

		auto blockChainConfigPath = resourcesPath / "config-network.properties";
		std::ifstream inputStream(blockChainConfigPath.generic_string());
		std::string serializedBlockChainConfig;
		inputStream.seekg(0, std::ios::end);
		serializedBlockChainConfig.reserve(inputStream.tellg());
		inputStream.seekg(0, std::ios::beg);
		serializedBlockChainConfig.assign(std::istreambuf_iterator<char>(inputStream), std::istreambuf_iterator<char>());
		setInitialBlockChainConfig(serializedBlockChainConfig);

		return m_currentLocalNodeConfig;
	}

	void LocalNodeConfigurationHolder::setInitialBlockChainConfig(const std::string& serializedBlockChainConfig) {
		m_blockChainConfigs.emplace(Height(1), serializedBlockChainConfig);
		update();
	}

	void LocalNodeConfigurationHolder::SetBlockChainConfig(const model::BlockChainConfiguration& config) {
		const_cast<model::BlockChainConfiguration&>(m_currentLocalNodeConfig.BlockChain) = config;
	}

	void LocalNodeConfigurationHolder::update() {
		if (m_blockChainConfigs.empty())
			return;

		std::istringstream configStream((--m_blockChainConfigs.end())->second);
		auto bag = utils::ConfigurationBag::FromStream(configStream);
		auto config = model::BlockChainConfiguration::LoadFromBag(bag);
		SetBlockChainConfig(config);
	}

	void LocalNodeConfigurationHolder::SetBlockChainConfig(const Height& height, const std::string& serializedBlockChainConfig) {
		if (height <= Height(1))
			CATAPULT_THROW_INVALID_ARGUMENT_1("invalid height", height);

		if (!!m_pDelta)
			m_pDelta->removeBlockChainConfig(height);

		m_blockChainConfigs.emplace(height, serializedBlockChainConfig);
		update();
	}

	void LocalNodeConfigurationHolder::RemoveBlockChainConfig(const Height& height) {
		if (!!m_pDelta && !!m_blockChainConfigs.count(height))
			m_pDelta->setBlockChainConfig(height, m_blockChainConfigs.at(height));

		m_blockChainConfigs.erase(height);
		update();
	}

	void LocalNodeConfigurationHolder::SaveBlockChainConfigs(io::OutputStream& output) {
		io::Write32(output, m_blockChainConfigs.size());
		for (const auto& pair : m_blockChainConfigs) {
			io::Write(output, pair.first);
			io::Write32(output, pair.second.size());
			io::Write(output, RawBuffer((const uint8_t*)pair.second.data(), pair.second.size()));
		}

		output.flush();
	}

	void LocalNodeConfigurationHolder::LoadBlockChainConfigs(io::InputStream& input) {
		m_blockChainConfigs.clear();
		auto blockChainConfigsSize = io::Read32(input);
		for (uint32_t i = 0; i < blockChainConfigsSize; ++i) {
			auto height = io::Read<Height>(input);
			auto blockChainConfigSize = io::Read32(input);
			std::string blockChainConfig;
			blockChainConfig.reserve(blockChainConfigSize);
			io::Read(input, MutableRawBuffer((uint8_t *) blockChainConfig.data(), blockChainConfig.size()));
			m_blockChainConfigs.emplace(height, blockChainConfig);
		}
		update();
	}

	std::unique_ptr<LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta> LocalNodeConfigurationHolder::CreateDelta() {
		return std::make_unique<LocalNodeConfigurationHolderDelta>(*this);
	}

	LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta::LocalNodeConfigurationHolderDelta(
			LocalNodeConfigurationHolder& configHolder) : m_configHolder(configHolder) {
		m_configHolder.m_pDelta = this;
	}

	LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta::~LocalNodeConfigurationHolderDelta() {
		for (const auto& pair : m_blockChainConfigs)
			m_configHolder.SetBlockChainConfig(pair.first, pair.second);
		m_configHolder.m_pDelta = nullptr;
	}

	void LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta::setBlockChainConfig(
			const Height& height, const std::string& serializedBlockChainConfig) {
		m_blockChainConfigs.emplace(height, serializedBlockChainConfig);
	}

	void LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta::removeBlockChainConfig(const Height& height) {
		m_blockChainConfigs.erase(height);
	}

	void LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta::Commit() {
		m_blockChainConfigs.clear();
	}
}}
