/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace config {

	BlockchainConfiguration GetMockNetworkConfig(const model::NetworkConfiguration& networkConfig)
	{
		test::MutableBlockchainConfiguration config;
		config.Network = networkConfig;
		return config.ToConst();
	}

	MockBlockchainConfigurationHolder::MockBlockchainConfigurationHolder()
			: BlockchainConfigurationHolder(nullptr)
	{}

	MockBlockchainConfigurationHolder::MockBlockchainConfigurationHolder(const model::NetworkConfiguration& networkConfig)
			: BlockchainConfigurationHolder(GetMockNetworkConfig(networkConfig)) {
	}

	MockBlockchainConfigurationHolder::MockBlockchainConfigurationHolder(const BlockchainConfiguration& config)
			: BlockchainConfigurationHolder(config) {
	}
	MockBlockchainConfigurationHolder::MockBlockchainConfigurationHolder(const BlockchainConfiguration& config, cache::CatapultCache* pCache, const Height& height)
		: BlockchainConfigurationHolder(config, pCache, height) {
	}

	const BlockchainConfiguration& MockBlockchainConfigurationHolder::Config(const Height& height) const {
		if(m_configs.find(height) != m_configs.end()) {
			return m_configs.at(height);
		}
		return m_configs.cbegin()->second;
	}

	const BlockchainConfiguration& MockBlockchainConfigurationHolder::Config() const {
		return m_configs.cbegin()->second;
	}

	const BlockchainConfiguration& MockBlockchainConfigurationHolder::ConfigAtHeightOrLatest(const Height&) const {
		return m_configs.cbegin()->second;
	}

	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolder() {
		return std::make_shared<MockBlockchainConfigurationHolder>();
	}
	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolderWithNemesisConfig() {
		auto holder = std::make_shared<MockBlockchainConfigurationHolder>();
		holder->InsertConfig(Height(1), holder->Config().Network, holder->Config().SupportedEntityVersions);
		return holder;
	}

	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolder(const model::NetworkConfiguration& config) {
		return std::make_shared<MockBlockchainConfigurationHolder>(config);
	}

	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolder(const BlockchainConfiguration& config) {
		return std::make_shared<MockBlockchainConfigurationHolder>(config);
	}
	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolderWithNemesisConfig(const BlockchainConfiguration& config) {
		auto holder = std::make_shared<MockBlockchainConfigurationHolder>(config);
		holder->InsertConfig(Height(1), config.Network, config.SupportedEntityVersions);
		return holder;
	}

	std::shared_ptr<MockBlockchainConfigurationHolder> CreateRealMockConfigurationHolder(const BlockchainConfiguration& config) {
		return std::make_shared<MockBlockchainConfigurationHolder>(config);
	}

	std::shared_ptr<MockBlockchainConfigurationHolder> CreateRealMockConfigurationHolderWithNemesisConfig(const BlockchainConfiguration& config) {
		auto holder = std::make_shared<MockBlockchainConfigurationHolder>(config);
		holder->InsertConfig(Height(1), config.Network, config.SupportedEntityVersions);
		return holder;
	}

	model::InflationCalculator& MockBlockchainConfigurationHolder::GetCalculator() {
		return *m_pInflationCalculator;
	}
}}
