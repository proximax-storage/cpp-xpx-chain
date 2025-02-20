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
		const_cast<config::ImmutableConfiguration*>(&m_configs.begin()->second.Immutable)->NemesisHeight = Height(1);
		m_InflationCalculator.add(Height(1), networkConfig.Inflation);
	}

	MockBlockchainConfigurationHolder::MockBlockchainConfigurationHolder(const BlockchainConfiguration& config)
			: BlockchainConfigurationHolder(config) {
		m_InflationCalculator.add(Height(1), config.Network.Inflation);
	}

	const BlockchainConfiguration& MockBlockchainConfigurationHolder::Config(const Height& height) const {
		if(m_configs.find(height) != m_configs.end()) {
			return m_configs.at(height);
		}
		return m_configs.at(Height{0});
	}

	const BlockchainConfiguration& MockBlockchainConfigurationHolder::Config() const {
		return m_configs.at(Height{0});
	}

	const BlockchainConfiguration& MockBlockchainConfigurationHolder::ConfigAtHeightOrLatest(const Height&) const {
		return m_configs.at(Height{0});
	}

	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolder() {
		return std::make_shared<MockBlockchainConfigurationHolder>();
	}

	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolder(const model::NetworkConfiguration& config) {
		return std::make_shared<MockBlockchainConfigurationHolder>(config);
	}

	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolder(const BlockchainConfiguration& config) {
		return std::make_shared<MockBlockchainConfigurationHolder>(config);
	}
	std::shared_ptr<MockBlockchainConfigurationHolder> CreateRealMockConfigurationHolder(const BlockchainConfiguration& config) {
		return std::make_shared<MockBlockchainConfigurationHolder>(config);
	}

	model::InflationCalculator& MockBlockchainConfigurationHolder::GetCalculator() {
		return m_InflationCalculator;
	}
}}
