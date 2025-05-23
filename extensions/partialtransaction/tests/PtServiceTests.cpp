/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "partialtransaction/src/PtService.h"
#include "partialtransaction/src/PtBootstrapperService.h"
#include "catapult/cache_tx/MemoryPtCache.h"
#include "catapult/model/TransactionFeeCalculator.h"
#include "tests/test/local/PacketWritersServiceTestUtils.h"
#include "tests/test/local/ServiceTestUtils.h"
#include "tests/test/net/mocks/MockPacketWriters.h"

namespace catapult { namespace partialtransaction {

#define TEST_CLASS PtServiceTests

	namespace {
		constexpr auto Num_Expected_Tasks = 2u;

		struct PtServiceTraits {
			static constexpr auto Counter_Name = "PT WRITERS";
			static constexpr auto Num_Expected_Services = 3u; // writers (1) + dependent services (2)

			static auto GetWriters(const extensions::ServiceLocator& locator) {
				return locator.service<net::PacketWriters>("api.partial");
			}

			static auto CreateRegistrar() {
				return CreatePtServiceRegistrar();
			}
		};

		class TestContext : public test::ServiceLocatorTestContext<PtServiceTraits> {
		public:
			TestContext() {
				// Arrange: register service dependencies
				auto pBootstrapperRegistrar = CreatePtBootstrapperServiceRegistrar([]() {
					return std::make_unique<cache::MemoryPtCacheProxy>(
							cache::MemoryCacheOptions(100, 100),
							std::make_shared<model::TransactionFeeCalculator>());
				});
				pBootstrapperRegistrar->registerServices(locator(), testState().state());

				// - register hook dependencies
				GetPtServerHooks(locator()).setCosignedTransactionInfosConsumer([](auto&&) {});
			}
		};

		struct Mixin {
			using TraitsType = PtServiceTraits;
			using TestContextType = TestContext;
		};
	}

	ADD_SERVICE_REGISTRAR_INFO_TEST(Pt, Post_Extended_Range_Consumers)

	ADD_PACKET_WRITERS_SERVICE_TEST(TEST_CLASS, Mixin, CanBootService)
	ADD_PACKET_WRITERS_SERVICE_TEST(TEST_CLASS, Mixin, CanShutdownService)
	ADD_PACKET_WRITERS_SERVICE_TEST(TEST_CLASS, Mixin, CanConnectToExternalServer)

	// region packetIoPickers

	TEST(TEST_CLASS, WritersAreRegisteredInPacketIoPickers) {
		// Arrange: create a (tcp) server
		auto pPool = test::CreateStartedIoThreadPool();
		auto serverKeyPair = test::GenerateKeyPair();
		test::SpawnPacketServerWork(pPool->ioContext(), [&serverKeyPair](const auto& pServer) {
			net::VerifyClient(pServer, serverKeyPair, ionet::ConnectionSecurityMode::None, [](auto, const auto&) {});
		});

		// Act: create and boot the service
		TestContext context;
		context.boot();
		auto pickers = context.testState().state().packetIoPickers();

		// - get the packet writers and attempt to connect to the server
		test::ConnectToLocalHost(*PtServiceTraits::GetWriters(context.locator()), serverKeyPair.publicKey());

		// Assert: the writers are registered with role `Api`
		EXPECT_EQ(0u, pickers.pickMatching(utils::TimeSpan::FromSeconds(1), ionet::NodeRoles::Peer).size());
		EXPECT_EQ(1u, pickers.pickMatching(utils::TimeSpan::FromSeconds(1), ionet::NodeRoles::Api).size());
	}

	// endregion

	// region tasks

	TEST(TEST_CLASS, ConnectPeersTaskIsScheduled) {
		// Assert:
		test::AssertRegisteredTask(TestContext(), Num_Expected_Tasks, "connect peers task for service Pt");
	}

	TEST(TEST_CLASS, PullPtTaskIsScheduled) {
		// Assert:
		test::AssertRegisteredTask(TestContext(), Num_Expected_Tasks, "pull partial transactions task");
	}

	// endregion
}}
