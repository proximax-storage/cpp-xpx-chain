/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "fastfinality/src/WeightedVotingService.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/core/mocks/MockCommitteeManager.h"
#include "tests/test/core/mocks/MockWeightedVotingFsm.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/cache/CacheTestUtils.h"

namespace catapult { namespace fastfinality {

#define TEST_CLASS TestWeightedVoting

	namespace {
		const char* Beneficiary_Key = "0000000000000000000000000000000000000000000000000000000000000000";

		auto mockBlockchainConfigurationHolder(bool enableWeightedVoting) {
			auto networkCfg = model::NetworkConfiguration::Uninitialized();
			networkCfg.EnableWeightedVoting = enableWeightedVoting;
			return config::CreateMockConfigurationHolder(networkCfg)->Config();
		}

		template<typename TAssert>
		void RunWeightedVotingService(
				std::shared_ptr<extensions::ServiceRegistrar> pWeightedVoting,
				config::MockBlockchainConfigurationHolder bcConfig,
				TAssert assertFn) {
			std::shared_ptr<mocks::MockCommitteeManager> committeeManager =
					std::make_shared<mocks::MockCommitteeManager>();
			extensions::ServiceLocator locator = extensions::ServiceLocator(
					crypto::KeyPair::FromPrivate(crypto::PrivateKey::Generate(test::RandomByte)));

			std::shared_ptr<test::ServiceTestState> serviceState = std::make_shared<test::ServiceTestState>(
					std::move(test::CreateEmptyCatapultCache()), bcConfig.Config());
			serviceState->pluginManager().setCommitteeManager(committeeManager);
			serviceState->state().hooks().setCompletionAwareBlockRangeConsumerFactory(
					[&](auto) { return [&](auto&& blockRange, auto) { return disruptor::DisruptorElementId(); }; });

			assertFn(pWeightedVoting, serviceState, locator);
		}

	}

	TEST(TEST_CLASS, WeightedVotingServiceRegistrar_Success) {
		harvesting::HarvestingConfiguration harvestingCfg = harvesting::HarvestingConfiguration::Uninitialized();
		harvestingCfg.Beneficiary = Beneficiary_Key;
		std::shared_ptr<extensions::ServiceRegistrar> pWeightedVoting =
				fastfinality::CreateWeightedVotingServiceRegistrar(harvestingCfg);

		RunWeightedVotingService(
				pWeightedVoting,
				mockBlockchainConfigurationHolder(true),
				[](auto pWeightedVoting, auto serviceState, auto locator) {
					auto& handlers = serviceState->state().packetHandlers();
					ASSERT_EQ(handlers.size(), 0);
					ASSERT_NO_THROW(pWeightedVoting->registerServices(locator, serviceState->state()));
					ASSERT_EQ(handlers.size(), 4);
					ASSERT_TRUE(handlers.canProcess(ionet::PacketType::Push_Proposed_Block));
					ASSERT_TRUE(handlers.canProcess(ionet::PacketType::Push_Confirmed_Block));
					ASSERT_TRUE(handlers.canProcess(ionet::PacketType::Push_Prevote_Message));
					ASSERT_TRUE(handlers.canProcess(ionet::PacketType::Push_Precommit_Message));
				});
	}

	TEST(TEST_CLASS, WeightedVotingServiceRegistrar_DisableWeightedVoting) {
		harvesting::HarvestingConfiguration harvestingCfg = harvesting::HarvestingConfiguration::Uninitialized();
		harvestingCfg.Beneficiary = Beneficiary_Key;
		std::shared_ptr<extensions::ServiceRegistrar> pWeightedVoting =
				fastfinality::CreateWeightedVotingServiceRegistrar(harvestingCfg);

		RunWeightedVotingService(
				pWeightedVoting,
				mockBlockchainConfigurationHolder(false),
				[](auto pWeightedVoting, auto serviceState, auto locator) {
					auto& handlers = serviceState->state().packetHandlers();
					ASSERT_EQ(handlers.size(), 0);
					ASSERT_THROW(
							pWeightedVoting->registerServices(locator, serviceState->state()), catapult_runtime_error);
					ASSERT_EQ(handlers.size(), 0);
					ASSERT_FALSE(handlers.canProcess(ionet::PacketType::Push_Proposed_Block));
					ASSERT_FALSE(handlers.canProcess(ionet::PacketType::Push_Confirmed_Block));
					ASSERT_FALSE(handlers.canProcess(ionet::PacketType::Push_Prevote_Message));
					ASSERT_FALSE(handlers.canProcess(ionet::PacketType::Push_Precommit_Message));
				});
	}

	TEST(TEST_CLASS, WeightedVotingStartServiceRegistrar_Success) {
		std::shared_ptr<extensions::ServiceRegistrar> pWeightedVoting =
				std::make_unique<mocks::MockWeightedVotingStartServiceRegistrar>();

		RunWeightedVotingService(
				pWeightedVoting,
				mockBlockchainConfigurationHolder(false),
				[&](auto pWeightedVoting, auto serviceState, auto locator) {
					std::shared_ptr<mocks::MockWeightedVotingFsm> pMockFsm =
							std::make_shared<mocks::MockWeightedVotingFsm>();
					locator.registerService(mocks::Service_Name, pMockFsm);

					ASSERT_EQ(pMockFsm->setPeerConnectionTasksCounter, 0);
					ASSERT_EQ(pMockFsm->startCounter, 0);
					pWeightedVoting->registerServices(locator, serviceState->state());
					ASSERT_EQ(pMockFsm->setPeerConnectionTasksCounter, 1);
					ASSERT_EQ(pMockFsm->startCounter, 1);
				});
	}
}}
