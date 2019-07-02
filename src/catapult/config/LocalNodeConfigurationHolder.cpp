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

	namespace {
		std::string LoadFileIntoString(boost::filesystem::path path) {
			std::ifstream inputStream(path.generic_string());
			std::string fileContent;
			inputStream.seekg(0, std::ios::end);
			fileContent.reserve(inputStream.tellg());
			inputStream.seekg(0, std::ios::beg);
			fileContent.assign(std::istreambuf_iterator<char>(inputStream), std::istreambuf_iterator<char>());

			return fileContent;
		}
	}

	// region LocalNodeConfigurationHolder

	LocalNodeConfigurationHolder::LocalNodeConfigurationHolder()
		: m_catapultConfig(
			model::BlockChainConfiguration::Uninitialized(),
			NodeConfiguration::Uninitialized(),
			LoggingConfiguration::Uninitialized(),
			UserConfiguration::Uninitialized(),
			SupportedEntityVersions())
		, m_blockChainConfigHolder(m_catapultConfig)
		, m_supportedEntityVersionsHolder(m_catapultConfig)
		, m_pDelta(nullptr)
	{}

	boost::filesystem::path LocalNodeConfigurationHolder::GetResourcesPath(int argc, const char** argv) {
		return boost::filesystem::path(argc > 1 ? argv[1] : "..") / "resources";
	}

	const LocalNodeConfiguration& LocalNodeConfigurationHolder::Config() const {
		return m_catapultConfig;
	}

	LocalNodeConfiguration& LocalNodeConfigurationHolder::Config() {
		return m_catapultConfig;
	}

	void LocalNodeConfigurationHolder::SetConfig(const LocalNodeConfiguration& config) {
		const_cast<model::BlockChainConfiguration&>(m_catapultConfig.BlockChain) = config.BlockChain;
		const_cast<NodeConfiguration&>(m_catapultConfig.Node) = config.Node;
		const_cast<LoggingConfiguration&>(m_catapultConfig.Logging) = config.Logging;
		const_cast<UserConfiguration&>(m_catapultConfig.User) = config.User;
		const_cast<SupportedEntityVersions&>(m_catapultConfig.SupportedEntityVersions) = config.SupportedEntityVersions;
	}

	const LocalNodeConfiguration& LocalNodeConfigurationHolder::LoadConfig(int argc, const char** argv) {
		auto resourcesPath = GetResourcesPath(argc, argv);
		std::cout << "loading resources from " << resourcesPath << std::endl;
		SetConfig(config::LocalNodeConfiguration::LoadFromPath(resourcesPath));

		m_blockChainConfigHolder.LoadInitialSubConfig(resourcesPath / "config-network.properties");
		m_supportedEntityVersionsHolder.LoadInitialSubConfig(resourcesPath / "supported-entities.json");

		return m_catapultConfig;
	}

	void LocalNodeConfigurationHolder::SetBlockChainConfig(const model::BlockChainConfiguration& config) {
		m_blockChainConfigHolder.SetSubConfig(config);
	}

	void LocalNodeConfigurationHolder::SetBlockChainConfig(const Height& height, const std::string& serializedBlockChainConfig) {
		m_blockChainConfigHolder.SetSubConfig(height, serializedBlockChainConfig);
	}

	void LocalNodeConfigurationHolder::RemoveBlockChainConfig(const Height& height) {
		m_blockChainConfigHolder.RemoveSubConfig(height);
	}

	void LocalNodeConfigurationHolder::SaveBlockChainConfigs(io::OutputStream& output) {
		m_blockChainConfigHolder.SaveSubConfigs(output);
	}

	void LocalNodeConfigurationHolder::LoadBlockChainConfigs(io::InputStream& input) {
		m_blockChainConfigHolder.LoadSubConfigs(input);
	}

	void LocalNodeConfigurationHolder::SetSupportedEntityVersions(const SupportedEntityVersions& config) {
		m_supportedEntityVersionsHolder.SetSubConfig(config);
	}

	void LocalNodeConfigurationHolder::SetSupportedEntityVersions(const Height& height, const std::string& serializedSupportedEntityVersions) {
		m_supportedEntityVersionsHolder.SetSubConfig(height, serializedSupportedEntityVersions);
	}

	void LocalNodeConfigurationHolder::RemoveSupportedEntityVersions(const Height& height) {
		m_supportedEntityVersionsHolder.RemoveSubConfig(height);
	}

	void LocalNodeConfigurationHolder::SaveSupportedEntityVersions(io::OutputStream& output) {
		m_supportedEntityVersionsHolder.SaveSubConfigs(output);
	}

	void LocalNodeConfigurationHolder::LoadSupportedEntityVersions(io::InputStream& input) {
		m_supportedEntityVersionsHolder.LoadSubConfigs(input);
	}

	std::unique_ptr<LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta> LocalNodeConfigurationHolder::CreateDelta() {
		return std::make_unique<LocalNodeConfigurationHolderDelta>(*this);
	}

	LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta::LocalNodeConfigurationHolderDelta(
		LocalNodeConfigurationHolder& configHolder) {
		m_pBlockChainConfigDelta = configHolder.m_blockChainConfigHolder.CreateDelta();
		m_pSupportedEntityVersionsDelta = configHolder.m_supportedEntityVersionsHolder.CreateDelta();
	}

	void LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta::Commit() {
		m_pBlockChainConfigDelta->Commit();
		m_pSupportedEntityVersionsDelta->Commit();
	}

	void LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta::setBlockChainConfig(
		const Height& height, const std::string& serializedBlockChainConfig) {
		m_pBlockChainConfigDelta->SetSubConfig(height, serializedBlockChainConfig);
	}

	void LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta::removeBlockChainConfig(const Height& height) {
		m_pBlockChainConfigDelta->RemoveSubConfig(height);
	}

	void LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta::setSupportedEntityVersions(
		const Height& height, const std::string& serializedSupportedEntityVersions) {
		m_pSupportedEntityVersionsDelta->SetSubConfig(height, serializedSupportedEntityVersions);
	}

	void LocalNodeConfigurationHolder::LocalNodeConfigurationHolderDelta::removeSupportedEntityVersions(const Height& height) {
		m_pSupportedEntityVersionsDelta->RemoveSubConfig(height);
	}

	// endregion

	// region SubConfigurationHolder

	template <typename TTraits>
	void SubConfigurationHolder<TTraits>::setInitialSubConfig(const std::string& serializedSubConfig) {
		m_serializedSubConfigs.emplace(Height(1), serializedSubConfig);
		update();
	}

	template <typename TTraits>
	void SubConfigurationHolder<TTraits>::SetSubConfig(const typename TTraits::ConfigType& subConfig) {
		TTraits::SetConfig(m_catapultConfig, subConfig);
	}

	template <typename TTraits>
	void SubConfigurationHolder<TTraits>::LoadInitialSubConfig(boost::filesystem::path path) {
		auto serializedSubConfig = LoadFileIntoString(path);
		setInitialSubConfig(serializedSubConfig);
	}

	template <typename TTraits>
	void SubConfigurationHolder<TTraits>::update() {
		if (!m_serializedSubConfigs.empty()) {
			std::istringstream configStream((--m_serializedSubConfigs.end())->second);
			auto subConfig = TTraits::LoadConfig(configStream);
			TTraits::SetConfig(m_catapultConfig, subConfig);
		}
	}

	template <typename TTraits>
	void SubConfigurationHolder<TTraits>::SetSubConfig(const Height& height, const std::string& serializedSubConfig) {
		if (height <= Height(1))
			CATAPULT_THROW_INVALID_ARGUMENT_1("invalid height", height);

		if (!!m_pDelta)
			m_pDelta->RemoveSubConfig(height);

		m_serializedSubConfigs.emplace(height, serializedSubConfig);
		update();
	}

	template <typename TTraits>
	void SubConfigurationHolder<TTraits>::RemoveSubConfig(const Height& height) {
		if (!!m_pDelta && !!m_serializedSubConfigs.count(height))
			m_pDelta->SetSubConfig(height, m_serializedSubConfigs.at(height));

		m_serializedSubConfigs.erase(height);
		update();
	}

	template <typename TTraits>
	void SubConfigurationHolder<TTraits>::SaveSubConfigs(io::OutputStream& output) {
		io::Write32(output, m_serializedSubConfigs.size());
		for (const auto& pair : m_serializedSubConfigs) {
			io::Write(output, pair.first);
			io::Write32(output, pair.second.size());
			io::Write(output, RawBuffer((const uint8_t*)pair.second.data(), pair.second.size()));
		}

		output.flush();
	}

	template <typename TTraits>
	void SubConfigurationHolder<TTraits>::LoadSubConfigs(io::InputStream& input) {
		m_serializedSubConfigs.clear();
		auto subConfigsSize = io::Read32(input);
		for (uint32_t i = 0; i < subConfigsSize; ++i) {
			auto height = io::Read<Height>(input);
			auto subConfigSize = io::Read32(input);
			std::string subConfig;
			subConfig.reserve(subConfigSize);
			io::Read(input, MutableRawBuffer((uint8_t *) subConfig.data(), subConfig.size()));
			m_serializedSubConfigs.emplace(height, subConfig);
		}
		update();
	}

	template <typename TTraits>
	std::unique_ptr<typename SubConfigurationHolder<TTraits>::SubConfigurationHolderDelta> SubConfigurationHolder<TTraits>::CreateDelta() {
		return std::make_unique<SubConfigurationHolderDelta>(*this);
	}

	template <typename TTraits>
	SubConfigurationHolder<TTraits>::SubConfigurationHolderDelta::SubConfigurationHolderDelta(
		SubConfigurationHolder<TTraits>& configHolder) : m_configHolder(configHolder) {
		m_configHolder.m_pDelta = this;
	}

	template <typename TTraits>
	SubConfigurationHolder<TTraits>::SubConfigurationHolderDelta::~SubConfigurationHolderDelta() {
		for (const auto& pair : m_serializedSubConfigs)
			m_configHolder.SetSubConfig(pair.first, pair.second);
		m_configHolder.m_pDelta = nullptr;
	}

	template <typename TTraits>
	void SubConfigurationHolder<TTraits>::SubConfigurationHolderDelta::SetSubConfig(
			const Height& height, const std::string& serializedBlockChainConfig) {
		m_serializedSubConfigs.emplace(height, serializedBlockChainConfig);
	}

	template <typename TTraits>
	void SubConfigurationHolder<TTraits>::SubConfigurationHolderDelta::RemoveSubConfig(const Height& height) {
		m_serializedSubConfigs.erase(height);
	}

	template <typename TTraits>
	void SubConfigurationHolder<TTraits>::SubConfigurationHolderDelta::Commit() {
		m_serializedSubConfigs.clear();
	}

	// endregion
}}
