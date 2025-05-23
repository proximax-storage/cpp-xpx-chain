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

#include "catapult/api/RemoteTransactionApi.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/test/other/RemoteApiFactory.h"
#include "tests/test/other/RemoteApiTestUtils.h"

namespace catapult { namespace api {

	namespace {
		using TransactionType = mocks::MockTransaction;

		std::shared_ptr<ionet::Packet> CreatePacketWithTransactions(uint16_t numTransactions) {
			// Arrange: create transactions with variable (incrementing) sizes
			uint32_t variableDataSize = numTransactions * (numTransactions + 1) / 2;
			uint32_t payloadSize = numTransactions * sizeof(TransactionType) + variableDataSize;
			auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
			test::FillWithRandomData({ pPacket->Data(), payloadSize });

			auto pData = pPacket->Data();
			for (uint16_t i = 0u; i < numTransactions; ++i, pData += sizeof(TransactionType) + i) {
				auto& transaction = reinterpret_cast<TransactionType&>(*pData);
				transaction.Size = sizeof(TransactionType) + i + 1;
				transaction.Type = TransactionType::Entity_Type;
				transaction.Deadline = Timestamp(5 * i);
				transaction.Data.Size = i + 1;
			}

			return pPacket;
		}

		struct UtTraits {
			static constexpr uint32_t Request_Data_Header_Size = sizeof(BlockFeeMultiplier);
			static constexpr uint32_t Request_Data_Size = 3 * sizeof(utils::ShortHash);

			static std::vector<uint32_t> KnownHashesValues() {
				return { 123, 234, 345 };
			}

			static model::ShortHashRange KnownShortHashes() {
				return model::ShortHashRange::CopyFixed(reinterpret_cast<uint8_t*>(KnownHashesValues().data()), 3);
			}

			static auto Invoke(const RemoteTransactionApi& api) {
				return api.unconfirmedTransactions(BlockFeeMultiplier(17), KnownShortHashes(), 10);
			}

			static auto CreateValidResponsePacket() {
				auto pResponsePacket = CreatePacketWithTransactions(3);
				pResponsePacket->Type = ionet::PacketType::Pull_Transactions;
				return pResponsePacket;
			}

			static auto CreateMalformedResponsePacket() {
				// the packet is malformed because it contains a partial transaction
				auto pResponsePacket = CreateValidResponsePacket();
				--pResponsePacket->Size;
				return pResponsePacket;
			}

			static void ValidateRequest(const ionet::Packet& packet) {
				EXPECT_EQ(ionet::PacketType::Pull_Transactions, packet.Type);
				ASSERT_EQ(sizeof(ionet::Packet) + Request_Data_Header_Size + Request_Data_Size, packet.Size);
				EXPECT_EQ(BlockFeeMultiplier(17), reinterpret_cast<const BlockFeeMultiplier&>(*packet.Data()));
				EXPECT_EQ_MEMORY(packet.Data() + sizeof(BlockFeeMultiplier), KnownHashesValues().data(), Request_Data_Size);
			}

			static void ValidateResponse(const ionet::Packet& response, const std::vector<model::TransactionRange>& transactions) {
				ASSERT_EQ(3u, transactions[0].size());

				auto pExpectedData = response.Data();
				auto parsedIter = transactions[0].cbegin();
				for (auto i = 0u; i < transactions.size(); ++i) {
					std::string message = "comparing transactions at " + std::to_string(i);
					const auto& expectedTransaction = reinterpret_cast<const TransactionType&>(*pExpectedData);
					const auto& actualTransaction = *parsedIter;
					ASSERT_EQ(expectedTransaction.Size, actualTransaction.Size) << message;
					EXPECT_EQ(Timestamp(5 * i), actualTransaction.Deadline) << message;
					EXPECT_EQ(expectedTransaction, actualTransaction) << message;
					++parsedIter;
					pExpectedData += expectedTransaction.Size;
				}
			}
		};

		struct RemoteTransactionApiTraits {
			static auto Create(ionet::PacketIo& packetIo, const Key& remotePublicKey) {
				auto registry = mocks::CreateDefaultTransactionRegistry();
				return test::CreateLifetimeExtendedApi(CreateRemoteTransactionApi, packetIo, remotePublicKey, std::move(registry));
			}

			static auto Create(ionet::PacketIo& packetIo) {
				return Create(packetIo, Key());
			}
		};
	}

	DEFINE_REMOTE_API_TESTS(RemoteTransactionApi)
	DEFINE_REMOTE_API_TESTS_EMPTY_RESPONSE_VALID(RemoteTransactionApi, Ut)
}}
