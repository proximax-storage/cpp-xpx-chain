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

#include "transactionsink/src/TransactionSinkService.h"
#include "tests/test/core/PacketTestUtils.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/local/ServiceTestUtils.h"

namespace catapult { namespace transactionsink {

#define TEST_CLASS TransactionSinkServiceTests

	namespace {
		struct TransactionSinkServiceTraits {
			static constexpr auto CreateRegistrar = CreateTransactionSinkServiceRegistrar;
		};

		class TestContext : public test::ServiceLocatorTestContext<TransactionSinkServiceTraits> {
		public:
			explicit TestContext(bool isChainSynced = true) : m_numPushedTransactionElements(0) {
				// set up hooks (only increment the counters if the input source is correct)
				auto& hooks = testState().state().hooks();
				hooks.setChainSyncedPredicate([isChainSynced]() { return isChainSynced; });
				hooks.setTransactionRangeConsumerFactory([&counter = m_numPushedTransactionElements](auto source) {
					return [&counter, source](auto&&) { counter += disruptor::InputSource::Remote_Push == source ? 1 : 0; };
				});

				// the service needs to be able to parse the mock transactions sent to it
				testState().pluginManager().addTransactionSupport(mocks::CreateMockTransactionPlugin());

				const_cast<config::NodeConfiguration&>(testState().config().Node).TransactionBatchSize = 50;
			}

		public:
			size_t numPushedTransactionElements() {
				return m_numPushedTransactionElements;
			}

		private:
			size_t m_numPushedTransactionElements;
		};
	}

	// region basic

	ADD_SERVICE_REGISTRAR_INFO_TEST(TransactionSink, Post_Range_Consumers)

	TEST(TEST_CLASS, NoServicesOrCountersAreRegistered) {
		// Assert:
		test::AssertNoServicesOrCountersAreRegistered<TestContext>();
	}

	TEST(TEST_CLASS, PacketHandlersAreRegistered) {
		// Assert:
		test::AssertSinglePacketHandlerIsRegistered<TestContext>(ionet::PacketType::Push_Transactions);
	}

	// endregion

	// region hook wiring

	namespace {
		void AssertTransactionPush(bool isChainSynced, size_t numExpectedPushes) {
			// Arrange:
			TestContext context(isChainSynced);
			context.boot();

			// Act:
			Key key{};
			ionet::ServerPacketHandlerContext handlerContext(key, "");
			const auto& handlers = context.testState().state().packetHandlers();
			handlers.process(*test::GenerateRandomTransactionPacket(), handlerContext);

			// Assert:
			EXPECT_EQ(numExpectedPushes, context.numPushedTransactionElements());
		}
	}

	TEST(TEST_CLASS, CanPushTransactionWhenSynced) {
		// Assert:
		AssertTransactionPush(true, 1);
	}

	TEST(TEST_CLASS, CannotPushTransactionWhenNotSynced) {
		// Assert:
		AssertTransactionPush(false, 0);
	}

	// endregion
}}
