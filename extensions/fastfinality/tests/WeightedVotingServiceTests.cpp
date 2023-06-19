/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "fastfinality/src/WeightedVotingService.h"
#include "catapult/handlers/HandlerFactory.h"
#include "catapult/net/PacketReaders.h"
#include "tests/test/core/mocks/MockCommitteeManager.h"
#include "tests/test/core/mocks/MockDbrbViewFetcher.h"
#include "tests/test/local/NetworkTestUtils.h"
#include "tests/test/local/ServiceLocatorTestContext.h"

namespace catapult { namespace fastfinality {

#define TEST_CLASS WeightedVotingServiceTests

	namespace {
		constexpr auto Readers_Counter_Name = "WV READERS";
		constexpr auto Writers_Counter_Name = "WV WRITERS";
		constexpr auto Writers_Service_Name = "weightedvoting.writers";
		constexpr auto Readers_Service_Name = "weightedvoting.readers";
		constexpr unsigned int Port_Diff = 3u;

		struct WeightedVotingServiceTraits {
			static constexpr auto CreateRegistrar = CreateWeightedVotingServiceRegistrar;
		};

		using TestContext = test::ServiceLocatorTestContext<WeightedVotingServiceTraits>;

		void boot(TestContext& context) {
			context.testState().pluginManager().setCommitteeManager(std::make_shared<mocks::MockCommitteeManager>());
			context.testState().pluginManager().setDbrbViewFetcher(std::make_shared<mocks::MockDbrbViewFetcher>());
			context.testState().state().hooks().setTransactionRangeConsumerFactory([](auto) { return [](auto&&) {}; });
			context.testState().state().hooks().setCompletionAwareBlockRangeConsumerFactory([](auto) {
				return [](auto&&, auto) { return disruptor::DisruptorElementId(); };
			});
			const_cast<bool&>(context.testState().config().Network.EnableWeightedVoting) = true;
			auto config = harvesting::HarvestingConfiguration::Uninitialized();
			config.Beneficiary = "0000000000000000000000000000000000000000000000000000000000000000";

			auto dbrbConfig = dbrb::DbrbConfiguration::Uninitialized();
			context.boot(config, dbrbConfig, "");
		}
	}

	// region boot + shutdown

	TEST(TEST_CLASS, CanBootService) {
		// Arrange:
		TestContext context;

		// Act:
		boot(context);

		// Assert:
		EXPECT_EQ(2u, context.locator().numServices());
		EXPECT_EQ(2u, context.locator().counters().size());

		EXPECT_TRUE(!!context.locator().service<net::PacketReaders>(Readers_Service_Name));
		EXPECT_TRUE(!!context.locator().service<net::PacketWriters>(Writers_Service_Name));
		EXPECT_EQ(0u, context.counter(Readers_Counter_Name));
		EXPECT_EQ(0u, context.counter(Writers_Counter_Name));
	}

	TEST(TEST_CLASS, CanShutdownService) {
		// Arrange:
		TestContext context;

		// Act:
		boot(context);
		context.shutdown();

		// Assert:
		EXPECT_EQ(2u, context.locator().numServices());
		EXPECT_EQ(2u, context.locator().counters().size());

		EXPECT_FALSE(!!context.locator().service<net::PacketReaders>(Readers_Service_Name));
		EXPECT_FALSE(!!context.locator().service<net::PacketWriters>(Writers_Service_Name));
		EXPECT_EQ(static_cast<uint64_t>(extensions::ServiceLocator::Sentinel_Counter_Value), context.counter(Readers_Counter_Name));
		EXPECT_EQ(static_cast<uint64_t>(extensions::ServiceLocator::Sentinel_Counter_Value), context.counter(Writers_Counter_Name));
	}

	// endregion

	// region connections

//	TEST(TEST_CLASS, CanAcceptExternalConnection) {
//		// Arrange:
//		TestContext context;
//		boot(context);
//
//		// - connect to the server as a reader
//		auto pPool = test::CreateStartedIoThreadPool();
//		auto pIo = test::ConnectToLocalHost(pPool->ioContext(), test::GetLocalHostPort() + Port_Diff, context.publicKey());
//
//		// Assert: a single connection was accepted
//		EXPECT_EQ(1u, context.counter(Counter_Name));
//	}

	// endregion
}}
