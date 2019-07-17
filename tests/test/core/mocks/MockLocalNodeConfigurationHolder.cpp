/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MockLocalNodeConfigurationHolder.h"

namespace catapult { namespace config {

	MockLocalNodeConfigurationHolder::MockLocalNodeConfigurationHolder() : LocalNodeConfigurationHolder(nullptr)
	{}

	MockLocalNodeConfigurationHolder::MockLocalNodeConfigurationHolder(const model::BlockChainConfiguration& config)
			: LocalNodeConfigurationHolder(nullptr) {
		const_cast<model::BlockChainConfiguration&>(m_catapultConfigs.at(Height{0}).BlockChain) = config;
	}

	MockLocalNodeConfigurationHolder::MockLocalNodeConfigurationHolder(const CatapultConfiguration& config)
			: LocalNodeConfigurationHolder(nullptr) {
		SetConfig(Height{0}, config);
	}

	CatapultConfiguration& MockLocalNodeConfigurationHolder::Config(const Height&) {
		return m_catapultConfigs.at(Height{0});
	}

	std::shared_ptr<LocalNodeConfigurationHolder> CreateMockConfigurationHolder() {
		return config::CreateMockConfigurationHolder();
	}

	std::shared_ptr<LocalNodeConfigurationHolder> CreateMockConfigurationHolder(const model::BlockChainConfiguration& config) {
		auto pConfigHolder = config::CreateMockConfigurationHolder(config);
		return pConfigHolder;
	}

	std::shared_ptr<LocalNodeConfigurationHolder> CreateMockConfigurationHolder(const CatapultConfiguration& config) {
		auto pConfigHolder = config::CreateMockConfigurationHolder(config);
		return pConfigHolder;
	}
}}
