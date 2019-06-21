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

#include "catapult/ionet/PacketHandlers.h"
#include "tests/test/core/PacketPayloadTestUtils.h"
#include "tests/TestHarness.h"
#include <memory>

namespace catapult { namespace ionet {

#define TEST_CLASS PacketHandlersTests

	using PacketHandlers = ServerPacketHandlers;

	namespace {
		void RegisterHandler(PacketHandlers& handlers, uint32_t type) {
			handlers.registerHandler(static_cast<PacketType>(type), [](const auto&, const auto&) {});
		}

		// marker is split up so that each nibble corresponds to a registered handler (the rightmost marker nibble
		// corresponds to the last handler)
		// if a handler is called, its corresponding nibble is set to the 1-based call index (so call order can be
		// verified too)
		void RegisterHandlers(PacketHandlers& handlers, const std::vector<uint32_t>& types, uint32_t& marker) {
			if (types.size() > sizeof(marker) * 2)
				CATAPULT_THROW_RUNTIME_ERROR_1("marker is too small for requested number of handlers", marker);

			// Act:
			auto pCounter = std::make_shared<uint32_t>(1);
			uint32_t i = 0;
			for (auto type : types) {
				auto bits = (types.size() - i - 1) * 4u;
				handlers.registerHandler(static_cast<PacketType>(type), [bits, pCounter, &marker](const auto&, const auto&) {
					marker |= *pCounter << bits;
					++*pCounter;
				});
				++i;
			}
		}

		ServerPacketHandlerContext CreateDefaultContext() {
			return ServerPacketHandlerContext({}, "");
		}

		size_t ProcessPacket(PacketHandlers& handlers, uint32_t type) {
			// Arrange:
			Packet packet;
			packet.Type = static_cast<PacketType>(type);

			// Act:
			auto context = CreateDefaultContext();
			return handlers.process(packet, context);
		}
	}

	// region ServerPacketHandlerContext

	TEST(TEST_CLASS, ServerPacketHandlerContextInitiallyHasNoResponse) {
		// Act:
		auto key = test::GenerateRandomByteArray<Key>();
		auto host = std::string("alice.com");
		ServerPacketHandlerContext context(key, host);

		// Assert:
		EXPECT_FALSE(context.hasResponse());

		EXPECT_EQ(key, context.key());
		EXPECT_EQ("alice.com", context.host());
	}

	TEST(TEST_CLASS, ServerPacketHandlerCannotAccessUnsetResponse) {
		// Arrange:
		auto context = CreateDefaultContext();

		// Act + Assert:
		EXPECT_THROW(context.response(), catapult_runtime_error);
	}

	TEST(TEST_CLASS, ServerPacketHandlerContextCanHaveResponseSet) {
		// Arrange:
		auto pPacket = CreateSharedPacket<Packet>(25);
		auto key = test::GenerateRandomByteArray<Key>();
		auto host = std::string("alice.com");
		ServerPacketHandlerContext context(key, host);

		// Act:
		context.response(PacketPayload(pPacket));

		// Assert:
		EXPECT_TRUE(context.hasResponse());
		test::AssertPacketPayload(*pPacket, context.response());

		EXPECT_EQ(key, context.key());
		EXPECT_EQ("alice.com", context.host());
	}

	TEST(TEST_CLASS, ServerPacketHandlerContextCannotChangeExplicitlySetResponse) {
		// Arrange:
		auto pPacket = CreateSharedPacket<Packet>(25);
		auto context = CreateDefaultContext();
		context.response(PacketPayload(pPacket));

		// Act + Assert:
		EXPECT_THROW(context.response(PacketPayload(pPacket)), catapult_runtime_error);
	}

	TEST(TEST_CLASS, ServerPacketHandlerContextHasResponseWhenExplicitlyUnset) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto host = std::string("alice.com");
		ServerPacketHandlerContext context(key, host);

		// Act:
		context.response(PacketPayload());

		// Assert:
		EXPECT_TRUE(context.hasResponse());
		test::AssertPacketPayloadUnset(context.response());

		EXPECT_EQ(key, context.key());
		EXPECT_EQ("alice.com", context.host());
	}

	TEST(TEST_CLASS, ServerPacketHandlerContextCannotChangeExplicitlyUnsetResponse) {
		// Arrange:
		auto context = CreateDefaultContext();
		context.response(PacketPayload());

		// Act + Assert:
		EXPECT_THROW(context.response(PacketPayload()), catapult_runtime_error);
	}

	// endregion

	// region constructor

	TEST(TEST_CLASS, CanCreateHandlersWithDefaultOptions) {
		// Act:
		PacketHandlers handlers;

		// Assert:
		EXPECT_EQ(0u, handlers.size());
		EXPECT_EQ(0xFFFF'FFFFu, handlers.maxPacketDataSize());
	}

	TEST(TEST_CLASS, CanCreateHandlersWithCustomOptions) {
		// Act:
		PacketHandlers handlers(45'987);

		// Assert:
		EXPECT_EQ(0u, handlers.size());
		EXPECT_EQ(45'987u, handlers.maxPacketDataSize());
	}

	// endregion

	// region registerHandler

	TEST(TEST_CLASS, CanAddSingleHandler) {
		// Act:
		PacketHandlers handlers;
		RegisterHandler(handlers, 1);

		// Assert:
		EXPECT_EQ(1u, handlers.size());
	}

	TEST(TEST_CLASS, CanAddMultipleHandlers) {
		// Act:
		auto marker = 0u;
		PacketHandlers handlers;
		RegisterHandlers(handlers, { 1, 2, 7 }, marker);

		// Assert:
		EXPECT_EQ(3u, handlers.size());
	}

	TEST(TEST_CLASS, CannotAddMultipleHandlersForSamePacketType) {
		// Arrange:
		auto marker = 0u;
		PacketHandlers handlers;
		RegisterHandlers(handlers, { 1, 2 }, marker);

		// Act + Assert:
		EXPECT_THROW(RegisterHandler(handlers, 1), catapult_runtime_error);
	}

	// endregion

	// region canProcess

	TEST(TEST_CLASS, CanProcessPacketTypeReturnsTrueForPacketWithRegisteredHandler) {
		// Arrange:
		uint32_t marker = 0u;
		PacketHandlers handlers;
		RegisterHandlers(handlers, { 1, 3, 5 }, marker);

		for (auto type : { 1, 3, 5 }) {
			// Act:
			auto canProcess = handlers.canProcess(static_cast<PacketType>(type));

			// Assert:
			EXPECT_TRUE(canProcess) << "type " << type;
		}
	}

	TEST(TEST_CLASS, CanProcessPacketTypeReturnsFalseForPacketWithNoRegisteredHandler) {
		// Arrange:
		uint32_t marker = 0u;
		PacketHandlers handlers;
		RegisterHandlers(handlers, { 1, 3, 5 }, marker);

		for (auto type : { 0, 2, 4, 7 }) {
			// Act:
			auto canProcess = handlers.canProcess(static_cast<PacketType>(type));

			// Assert:
			EXPECT_FALSE(canProcess) << "type " << type;
		}
	}

	TEST(TEST_CLASS, CanProcessPacketReturnsTrueForPacketWithRegisteredHandler) {
		// Arrange:
		uint32_t marker = 0u;
		PacketHandlers handlers;
		RegisterHandlers(handlers, { 1, 3, 5 }, marker);

		for (auto type : { 1, 3, 5 }) {
			// Act:
			Packet packet;
			packet.Type = static_cast<PacketType>(type);
			auto canProcess = handlers.canProcess(packet);

			// Assert:
			EXPECT_TRUE(canProcess) << "type " << type;
		}
	}

	TEST(TEST_CLASS, CanProcessPacketReturnsFalseForPacketWithNoRegisteredHandler) {
		// Arrange:
		uint32_t marker = 0u;
		PacketHandlers handlers;
		RegisterHandlers(handlers, { 1, 3, 5 }, marker);

		for (auto type : { 0, 2, 4, 7 }) {
			// Act:
			Packet packet;
			packet.Type = static_cast<PacketType>(type);
			auto canProcess = handlers.canProcess(packet);

			// Assert:
			EXPECT_FALSE(canProcess) << "type " << type;
		}
	}

	// endregion

	TEST(TEST_CLASS, PacketHandlerForZeroTypeIsNotInitiallyRegistered) {
		// Act:
		Packet packet;
		packet.Type = static_cast<PacketType>(0);
		PacketHandlers handlers;

		// Assert:
		EXPECT_EQ(0u, handlers.size());
		EXPECT_FALSE(handlers.canProcess(packet));
	}

	TEST(TEST_CLASS, CanAddHandlerForZeroType) {
		// Arrange:
		Packet packet;
		packet.Type = static_cast<PacketType>(0);
		PacketHandlers handlers;

		// Act:
		RegisterHandler(handlers, 0);

		// Assert:
		EXPECT_EQ(1u, handlers.size());
		EXPECT_TRUE(handlers.canProcess(packet));
	}

	namespace {
		void AssertNoMatchingHandlers(uint32_t type) {
			// Arrange:
			uint32_t marker = 0u;
			PacketHandlers handlers;
			RegisterHandlers(handlers, { 1, 3, 5 }, marker);

			// Act:
			auto isProcessed = ProcessPacket(handlers, type);

			// Assert:
			EXPECT_FALSE(isProcessed);
			EXPECT_EQ(0x00000000u, marker);
		}
	}

	TEST(TEST_CLASS, CanProcessPacketWithZeroMatchingHandlers) {
		// Assert:
		AssertNoMatchingHandlers(4); // empty slot
		AssertNoMatchingHandlers(44); // beyond end
	}

	TEST(TEST_CLASS, CanProcessPacketWithSingleMatchingHandler) {
		// Arrange:
		auto marker = 0u;
		PacketHandlers handlers;
		RegisterHandlers(handlers, { 1, 3, 5 }, marker);

		// Act:
		auto isProcessed = ProcessPacket(handlers, 3);

		// Assert:
		EXPECT_TRUE(isProcessed);
		EXPECT_EQ(0x00000010u, marker);
	}

	TEST(TEST_CLASS, PacketIsPassedToHandler) {
		// Arrange:
		constexpr uint32_t Packet_Size = 25;
		PacketHandlers handlers;
		ByteBuffer packetBufferPassedToHandler(Packet_Size);
		handlers.registerHandler(static_cast<PacketType>(1), [&](const auto& packet, const auto&) {
			std::memcpy(&packetBufferPassedToHandler[0], &packet, Packet_Size);
		});

		// Act:
		auto packetBuffer = test::GenerateRandomVector(Packet_Size);
		auto pPacket = reinterpret_cast<Packet*>(&packetBuffer[0]);
		pPacket->Size = Packet_Size;
		pPacket->Type = static_cast<PacketType>(1);
		auto handlerContext = CreateDefaultContext();
		handlers.process(*pPacket, handlerContext);

		// Assert:
		EXPECT_EQ(packetBuffer, packetBufferPassedToHandler);
	}

	TEST(TEST_CLASS, MatchingHandlerCanRespondViaContext) {
		// Arrange: set up 3 handlers for packet types 1, 2, 3
		//          the handler will write the packet type to the response packet based on the matching type
		constexpr uint32_t Num_Handlers = 3;
		PacketHandlers handlers;
		auto numCallbackCalls = 0u;
		for (auto i = 0u; i < Num_Handlers; ++i) {
			handlers.registerHandler(static_cast<PacketType>(i), [i, &numCallbackCalls](const auto&, auto& context) {
				auto pResponsePacket = CreateSharedPacket<Packet>();
				pResponsePacket->Type = static_cast<PacketType>(0xFF ^ (1 << i));
				context.response(PacketPayload(pResponsePacket));
				++numCallbackCalls;
			});
		}

		// Act:
		Packet packet;
		packet.Size = sizeof(Packet);
		packet.Type = static_cast<PacketType>(2);
		auto handlerContext = CreateDefaultContext();
		handlers.process(packet, handlerContext);

		// Assert:
		EXPECT_EQ(1u, numCallbackCalls);
		EXPECT_EQ(static_cast<PacketType>(0xFB), handlerContext.response().header().Type);
	}
}}
