/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config_holder/LocalNodeConfigurationHolder.h"

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace config {

	class MockLocalNodeConfigurationHolder : public LocalNodeConfigurationHolder {
	public:
		MockLocalNodeConfigurationHolder() : LocalNodeConfigurationHolder(nullptr)
		{}

		MockLocalNodeConfigurationHolder(const model::BlockChainConfiguration& config) : LocalNodeConfigurationHolder(nullptr) {
			SetBlockChainConfig(config);
		}

	public:
		LocalNodeConfiguration& Config(const Height&) override {
			return m_catapultConfigs.at(Height{0});
		}

		void SetBlockChainConfig(const model::BlockChainConfiguration& config) {
			const_cast<model::BlockChainConfiguration&>(m_catapultConfigs.at(Height{0}).BlockChain) = config;
		}
	};
}}
