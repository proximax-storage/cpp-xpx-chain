/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace config {

	class MockBlockchainConfigurationHolder : public BlockchainConfigurationHolder {
	public:
		MockBlockchainConfigurationHolder();
		MockBlockchainConfigurationHolder(const model::NetworkConfiguration& config);
		MockBlockchainConfigurationHolder(const BlockchainConfiguration& config);
		MockBlockchainConfigurationHolder(const BlockchainConfiguration& config, cache::CatapultCache* pCache, const Height& height);

	public:
		const BlockchainConfiguration& Config(const Height&) const override;
		const BlockchainConfiguration& Config() const override;
		const BlockchainConfiguration& ConfigAtHeightOrLatest(const Height&) const override;
		model::InflationCalculator& GetCalculator();
		using BlockchainConfigurationHolder::InsertConfig;
	};

	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolder();
			std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolderWithNemesisConfig();
	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolder(const model::NetworkConfiguration& config);
	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolder(const BlockchainConfiguration& config);
	std::shared_ptr<BlockchainConfigurationHolder> CreateMockConfigurationHolderWithNemesisConfig(const BlockchainConfiguration& config);
	std::shared_ptr<MockBlockchainConfigurationHolder> CreateRealMockConfigurationHolder(const BlockchainConfiguration& config);
	std::shared_ptr<MockBlockchainConfigurationHolder> CreateRealMockConfigurationHolderWithNemesisConfig(const BlockchainConfiguration& config);
}}
