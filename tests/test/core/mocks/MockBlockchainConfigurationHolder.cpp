/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace config {

	MockBlockchainConfigurationHolder::MockBlockchainConfigurationHolder()
			: BlockchainConfigurationHolder(nullptr)
	{}

	MockBlockchainConfigurationHolder::MockBlockchainConfigurationHolder(const model::NetworkConfiguration& networkConfig)
			: BlockchainConfigurationHolder(nullptr) {
		test::MutableBlockchainConfiguration config;
		config.Network = networkConfig;
		SetConfig(Height{0}, config.ToConst());
	}

	MockBlockchainConfigurationHolder::MockBlockchainConfigurationHolder(const BlockchainConfiguration& config)
			: BlockchainConfigurationHolder(nullptr) {
		SetConfig(Height{0}, config);
	}

	const BlockchainConfiguration& MockBlockchainConfigurationHolder::Config(const Height&) {
		return m_configs.at(Height{0});
	}

	const BlockchainConfiguration& MockBlockchainConfigurationHolder::Config() {
		return m_configs.at(Height{0});
	}

	const BlockchainConfiguration& MockBlockchainConfigurationHolder::ConfigAtHeightOrLatest(const Height&) {
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
}}
