/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "FastFinalityUtils.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/handlers/ChainHandlers.h"
#include "catapult/harvesting_core/HarvestingConfiguration.h"
#include "catapult/harvesting_core/HarvestingUtFacadeFactory.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"

namespace catapult { namespace fastfinality {

	std::shared_ptr<harvesting::UnlockedAccounts> CreateUnlockedAccounts(const harvesting::HarvestingConfiguration& config) {
		auto pUnlockedAccounts = std::make_shared<harvesting::UnlockedAccounts>(config.MaxUnlockedAccounts);
		if (config.IsAutoHarvestingEnabled) {
			auto keyPair = crypto::KeyPair::FromString(config.HarvestKey);
			auto publicKey = keyPair.publicKey();

			auto unlockResult = pUnlockedAccounts->modifier().add(std::move(keyPair));
			CATAPULT_LOG(info) << "Added account " << publicKey << " for harvesting with result " << unlockResult;
		}

		return pUnlockedAccounts;
	}

	harvesting::BlockGenerator CreateHarvesterBlockGenerator(extensions::ServiceState& state) {
		auto strategy = state.config().Node.TransactionSelectionStrategy;
		auto executionConfig = extensions::CreateExecutionConfiguration(state.pluginManager());
		harvesting::HarvestingUtFacadeFactory utFacadeFactory(state.cache(), executionConfig);

		return harvesting::CreateHarvesterBlockGenerator(strategy, utFacadeFactory, state.utCache());
	}

	handlers::PullBlocksHandlerConfiguration CreatePullBlocksHandlerConfiguration(const config::NodeConfiguration& nodeConfig) {
		handlers::PullBlocksHandlerConfiguration config {
			nodeConfig.MaxBlocksPerSyncAttempt,
			nodeConfig.MaxChainBytesPerSyncAttempt.bytes32(),
		};

		return config;
	}
}}