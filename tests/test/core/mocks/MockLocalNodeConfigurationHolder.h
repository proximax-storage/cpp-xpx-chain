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
		MockLocalNodeConfigurationHolder();
		MockLocalNodeConfigurationHolder(const model::BlockChainConfiguration& config);
		MockLocalNodeConfigurationHolder(const CatapultConfiguration& config);

	public:
		CatapultConfiguration& Config(const Height&) override;
	};

	std::shared_ptr<LocalNodeConfigurationHolder> CreateMockConfigurationHolder();
	std::shared_ptr<LocalNodeConfigurationHolder> CreateMockConfigurationHolder(const model::BlockChainConfiguration& config);
	std::shared_ptr<LocalNodeConfigurationHolder> CreateMockConfigurationHolder(const CatapultConfiguration& config);
}}
