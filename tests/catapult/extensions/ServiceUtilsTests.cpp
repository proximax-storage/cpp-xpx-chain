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

#include "catapult/extensions/ServiceUtils.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ServerHooks.h"
#include "catapult/extensions/ServiceLocator.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/PacketPayloadTestUtils.h"
#include "tests/test/core/TransactionInfoTestUtils.h"
#include "tests/test/net/mocks/MockPacketWriters.h"
#include "tests/TestHarness.h"

namespace catapult { namespace extensions {

#define TEST_CLASS ServiceUtilsTests

	namespace {
		constexpr auto Service_Name = "writers";

		using CosignaturesSink = consumer<const std::vector<model::DetachedCosignature>&>;
		using TransactionSink = extensions::SharedNewTransactionsSink;
	}

	TEST(TEST_CLASS, CanCreatePushEntitySink) {
		// Arrange:
		auto pWriters = std::make_shared<mocks::BroadcastAwareMockPacketWriters>();
		auto keyPair = test::GenerateKeyPair();
		auto transactionInfos = test::CreateTransactionInfos(1);
		auto expectedPayload = ionet::CreateBroadcastPayload(transactionInfos);
		ServiceLocator locator(keyPair);
		locator.registerService(Service_Name, pWriters);

		// Act:
		auto sink = CreatePushEntitySink<TransactionSink>(locator, Service_Name);
		sink(transactionInfos);

		// Assert:
		ASSERT_EQ(1u, pWriters->broadcastedPayloads().size());
		EXPECT_EQ(ionet::PacketType::Push_Transactions, pWriters->broadcastedPayloads()[0].header().Type);
		test::AssertEqualPayload(expectedPayload, pWriters->broadcastedPayloads()[0]);
	}

	TEST(TEST_CLASS, CanCreatePushEntitySinkWithCustomPacketType) {
		// Arrange:
		auto pWriters = std::make_shared<mocks::BroadcastAwareMockPacketWriters>();
		auto keyPair = test::GenerateKeyPair();
		auto transactionInfos = test::CreateTransactionInfos(1);
		auto expectedPayload = ionet::CreateBroadcastPayload(transactionInfos, ionet::PacketType::Push_Partial_Transactions);
		ServiceLocator locator(keyPair);
		locator.registerService(Service_Name, pWriters);

		// Act:
		auto sink = CreatePushEntitySink<TransactionSink>(locator, Service_Name, ionet::PacketType::Push_Partial_Transactions);
		sink(transactionInfos);

		// Assert:
		ASSERT_EQ(1u, pWriters->broadcastedPayloads().size());
		EXPECT_EQ(ionet::PacketType::Push_Partial_Transactions, pWriters->broadcastedPayloads()[0].header().Type);
		test::AssertEqualPayload(expectedPayload, pWriters->broadcastedPayloads()[0]);
	}
}}
