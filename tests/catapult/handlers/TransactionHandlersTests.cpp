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

#include "catapult/handlers/TransactionHandlers.h"
#include "tests/test/core/EntityTestUtils.h"
#include "tests/test/core/PushHandlerTestUtils.h"
#include "tests/test/plugins/PullHandlerTests.h"

namespace catapult { namespace handlers {

#define TEST_CLASS TransactionHandlersTests

	// region PushTransactionsHandler

	namespace {
		struct PushTransactionsTraits {
			static constexpr auto Packet_Type = ionet::PacketType::Push_Transactions;
			static constexpr auto Data_Size = sizeof(mocks::MockTransaction);

			static constexpr size_t AdditionalPacketSize(size_t numTransactions) {
				return numTransactions * (numTransactions + 1) / 2;
			}

			static void PreparePacket(ionet::ByteBuffer& buffer, size_t count) {
				auto currentOffset = sizeof(ionet::Packet);
				for (auto i = 0u; i < count; ++i) {
					auto size = Data_Size + i + 1;
					test::SetTransactionAt(buffer, currentOffset, size);
					currentOffset += size;
				}
			}

			static auto CreateRegistry() {
				model::TransactionRegistry registry;
				registry.registerPlugin(mocks::CreateMockTransactionPlugin());
				return registry;
			}

			static auto RegisterHandler(
					ionet::ServerPacketHandlers& handlers,
					const model::TransactionRegistry& registry,
					const TransactionRangeHandler& rangeHandler) {
				return RegisterPushTransactionsHandler(handlers, registry, rangeHandler, 10);
			}
		};
	}

	DEFINE_PUSH_HANDLER_TESTS(TEST_CLASS, PushTransactions)

	// endregion

	// region PullTransactionsHandler - basic edge case tests

	namespace {
		struct PullTransactionsTraits {
			static constexpr auto Data_Header_Size = sizeof(BlockFeeMultiplier);
			static constexpr auto Packet_Type = ionet::PacketType::Pull_Transactions;
			static constexpr auto Valid_Request_Payload_Size = sizeof(utils::ShortHash);

			using ResponseType = UnconfirmedTransactions;
			using RetrieverParamType = utils::ShortHashesSet;

			using UtRetrieverAdapter = std::function<UnconfirmedTransactions (const utils::ShortHashesSet&)>;
			static auto RegisterHandler(ionet::ServerPacketHandlers& handlers, const UtRetrieverAdapter& utRetriever) {
				handlers::RegisterPullTransactionsHandler(handlers, [utRetriever](auto, const auto& knownShortHashes){
					return utRetriever(knownShortHashes);
				});
			}
		};

		DEFINE_PULL_HANDLER_EDGE_CASE_TESTS(TEST_CLASS, PullTransactions)
	}

	// endregion

	// region PullTransactionsHandler - request + response tests

	namespace {
		static auto ExtractFromPacket(const ionet::Packet& packet, size_t numRequestHashes) {
			auto minFeeMultiplier = reinterpret_cast<const BlockFeeMultiplier&>(*packet.Data());

			utils::ShortHashesSet extractedShortHashes;
			auto pShortHashData = reinterpret_cast<const utils::ShortHash*>(packet.Data() + sizeof(BlockFeeMultiplier));
			for (auto i = 0u; i < numRequestHashes; ++i)
				extractedShortHashes.insert(*pShortHashData++);

			return std::make_pair(minFeeMultiplier, extractedShortHashes);
		}

		class PullResponseContext {
		public:
			explicit PullResponseContext(size_t numResponseTransactions) {
				for (uint16_t i = 0u; i < numResponseTransactions; ++i)
					m_transactions.push_back(mocks::CreateMockTransaction(i + 1));
			}

		public:
			const auto& response() const {
				return m_transactions;
			}

			auto responseSize() const {
				return test::TotalSize(m_transactions);
			}

			void assertPayload(const ionet::PacketPayload& payload) {
				ASSERT_EQ(m_transactions.size(), payload.buffers().size());

				auto i = 0u;
				for (const auto& pExpectedTransaction : m_transactions) {
					const auto& transaction = reinterpret_cast<const mocks::MockTransaction&>(*payload.buffers()[i++].pData);
					EXPECT_EQ(*pExpectedTransaction, transaction);
				}
			}

		private:
			UnconfirmedTransactions m_transactions;
		};

		void AssertPullResponseIsSetWhenPacketIsValid(uint32_t numRequestHashes, uint32_t numResponseTransactions) {
			// Arrange:
			auto packetType = PullTransactionsTraits::Packet_Type;
			auto pPacket = test::CreateRandomPacket(sizeof(BlockFeeMultiplier) + numRequestHashes * sizeof(utils::ShortHash), packetType);
			ionet::ServerPacketHandlers handlers;
			size_t counter = 0;

			auto extractedRequestData = ExtractFromPacket(*pPacket, numRequestHashes);
			BlockFeeMultiplier actualFeeMultiplier;
			utils::ShortHashesSet actualRequestHashes;
			PullResponseContext responseContext(numResponseTransactions);
			RegisterPullTransactionsHandler(handlers, [&](auto minFeeMultiplier, const auto& requestHashes) {
				++counter;
				actualFeeMultiplier = minFeeMultiplier;
				actualRequestHashes = requestHashes;
				return responseContext.response();
			});

			// Act:
			ionet::ServerPacketHandlerContext context({}, "");
			EXPECT_TRUE(handlers.process(*pPacket, context));

			// Assert: the requested values were passed to the supplier
			EXPECT_EQ(extractedRequestData.first, actualFeeMultiplier);
			EXPECT_EQ(extractedRequestData.second, actualRequestHashes);

			// - the handler was called and has the correct header
			EXPECT_EQ(1u, counter);
			ASSERT_TRUE(context.hasResponse());
			auto payload = context.response();
			test::AssertPacketHeader(payload, sizeof(ionet::PacketHeader) + responseContext.responseSize(), packetType);

			// - let the traits assert the returned payload (may be one or more buffers)
			responseContext.assertPayload(payload);
		}
	}

	DEFINE_PULL_HANDLER_REQUEST_RESPONSE_TESTS(TEST_CLASS, AssertPullResponseIsSetWhenPacketIsValid)

	// endregion
}}
