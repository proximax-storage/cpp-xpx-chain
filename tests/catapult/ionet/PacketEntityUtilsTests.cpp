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

#include "catapult/ionet/PacketEntityUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/PacketTestUtils.h"

namespace catapult { namespace ionet {

#define TEST_CLASS PacketEntityUtilsTests

	// region CalculatePacketDataSize

	TEST(TEST_CLASS, CalculatePacketDataSizeReturnsZeroWhenPacketIsTooSmall) {
		// Arrange:
		for (auto size : std::initializer_list<uint32_t>{ 0, sizeof(PacketHeader) - 1 }) {
			Packet packet;
			packet.Size = size;

			// Act + Assert:
			EXPECT_EQ(0u, CalculatePacketDataSize(packet));
		}
	}

	TEST(TEST_CLASS, CalculatePacketDataSizeReturnsZeroWhenPacketIsHeaderOnly) {
		// Arrange:
		Packet packet;
		packet.Size = sizeof(PacketHeader);

		// Act + Assert:
		EXPECT_EQ(0u, CalculatePacketDataSize(packet));
	}

	TEST(TEST_CLASS, CalculatePacketDataSizeReturnsDataSizeWhenPacketContainsData) {
		// Arrange:
		for (auto dataSize : std::initializer_list<uint32_t>{ 1, 100 }) {
			Packet packet;
			packet.Size = sizeof(PacketHeader) + dataSize;

			// Act + Assert:
			EXPECT_EQ(dataSize, CalculatePacketDataSize(packet));
		}
	}

	// endregion

	// region IsSizeValid

	namespace {
#pragma pack(push, 1)

		struct VariableSizedEntity {
		public:
			explicit VariableSizedEntity(uint8_t extraSize) : ExtraSize(extraSize)
			{}

		public:
			uint32_t Size;
			uint8_t ExtraSize;

		public:
			static uint64_t CalculateRealSize(const VariableSizedEntity& entity) {
				return sizeof(VariableSizedEntity) + entity.ExtraSize;
			}
		};

#pragma pack(pop)
	}

	TEST(TEST_CLASS, IsSizeValidReturnsTrueWhenEntitySizeIsCorrect) {
		// Arrange:
		auto entity = VariableSizedEntity(123);
		entity.Size = sizeof(VariableSizedEntity) + 123;

		// Act + Assert:
		EXPECT_TRUE(IsSizeValid(entity));
	}

	TEST(TEST_CLASS, IsSizeValidReturnsFalseWhenEntitySizeIsTooSmall) {
		// Arrange:
		auto entity = VariableSizedEntity(123);
		entity.Size = sizeof(VariableSizedEntity) + 123 - 1;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(entity));
	}

	TEST(TEST_CLASS, IsSizeValidReturnsFalseWhenEntitySizeIsTooLarge) {
		// Arrange:
		auto entity = VariableSizedEntity(123);
		entity.Size = sizeof(VariableSizedEntity) + 123 + 1;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(entity));
	}

	// endregion

	// region ExtractEntitiesFromPacket / ExtractEntityFromPacket

	namespace {
		constexpr auto Default_Packet_Type = PacketType::Push_Block; // packet type is not validated by the parser
		constexpr uint32_t Transaction_Size = sizeof(mocks::MockTransaction);
		constexpr uint32_t Block_Packet_Size = sizeof(Packet) + sizeof(model::BlockHeaderV4);
		constexpr uint32_t Block_Transaction_Size = sizeof(model::BlockHeaderV4) + Transaction_Size;
		constexpr uint32_t Block_Transaction_Packet_Size = sizeof(Packet) + Block_Transaction_Size;

		void SetTransactionAt(ByteBuffer& buffer, size_t offset) {
			test::SetTransactionAt(buffer, offset, Transaction_Size);
		}

		struct ExtractEntityTraits {
			template<typename TIsValidPredicate>
			static auto Extract(const Packet& packet, TIsValidPredicate isValid) {
				return ExtractEntityFromPacket<model::Block>(packet, isValid);
			}

			static auto Extract(const Packet& packet) {
				return Extract(packet, test::DefaultSizeCheck<model::Block>);
			}

			static bool IsEmpty(const model::UniqueEntityPtr<model::Block>& pBlock) {
				return !pBlock;
			}

			static void Unwrap(const model::UniqueEntityPtr<model::Block>& pBlock, const model::Block*& pBlockOut) {
				ASSERT_TRUE(!!pBlock);
				pBlockOut = pBlock.get();
			}

			static Packet& CreatePacketWithOverflowSize(ByteBuffer& buffer, uint32_t size) {
				// create a packet with no complete blocks but some overflow bytes
				constexpr auto Base_Packet_Size = sizeof(Packet);
				buffer.resize(Base_Packet_Size + std::max<uint32_t>(sizeof(model::BlockHeaderV4), size));
				auto& packet = test::SetPushBlockPacketInBuffer(buffer);
				packet.Size = Base_Packet_Size + size;
				return packet;
			}
		};

		struct ExtractEntitiesTraits {
			template<typename TIsValidPredicate>
			static auto Extract(const Packet& packet, TIsValidPredicate isValid) {
				return ExtractEntitiesFromPacket<model::Block>(packet, isValid);
			}

			static auto Extract(const Packet& packet) {
				return Extract(packet, test::DefaultSizeCheck<model::Block>);
			}

			static bool IsEmpty(const model::BlockRange& range) {
				return range.empty();
			}

			static void Unwrap(const model::BlockRange& range, const model::Block*& pBlockOut) {
				ASSERT_EQ(1u, range.size());
				pBlockOut = range.data();
			}
		};

		struct ExtractEntitiesSingleEntityTraits : ExtractEntitiesTraits {
			static Packet& CreatePacketWithOverflowSize(ByteBuffer& buffer, uint32_t size) {
				return ExtractEntityTraits::CreatePacketWithOverflowSize(buffer, size);
			}
		};

		struct ExtractEntitiesMultiEntityTraits : ExtractEntitiesTraits {
			static Packet& CreatePacketWithOverflowSize(ByteBuffer& buffer, uint32_t size) {
				// create a packet with two complete blocks and some overflow bytes
				constexpr auto Num_Full_Blocks = 2u;
				constexpr auto Base_Packet_Size = sizeof(Packet) + Num_Full_Blocks * sizeof(model::BlockHeaderV4);
				buffer.resize(Base_Packet_Size + std::max<uint32_t>(sizeof(model::BlockHeaderV4), size));
				auto& packet = test::SetPushBlockPacketInBuffer(buffer);
				for (auto i = 0u; i <= Num_Full_Blocks; ++i) {
					auto blockSize = Num_Full_Blocks == i ? size : sizeof(model::BlockHeaderV4);
					test::SetBlockAt(buffer, sizeof(Packet) + i * sizeof(model::BlockHeaderV4), blockSize);
				}

				packet.Size = Base_Packet_Size + size;
				return packet;
			}
		};

		struct ExtractFixedSizeStructuresTraits {
			static auto Extract(const Packet& packet) {
				return ExtractFixedSizeStructuresFromPacket<Hash256>(packet);
			}

			static bool IsEmpty(const model::HashRange& range) {
				return range.empty();
			}
		};

		template<typename TTraits>
		void AssertCannotExtractFromPacketHeaderWithNoData(uint32_t size) {
			// Arrange:
			Packet packet;
			packet.Size = size;
			packet.Type = Default_Packet_Type;

			// Act:
			auto extractResult = TTraits::Extract(packet);

			// Assert:
			EXPECT_TRUE(TTraits::IsEmpty(extractResult)) << "packet size " << size;
		}

		template<typename TTraits>
		void AssertCannotExtractEntitiesFromPacketWithSize(uint32_t size) {
			// Arrange:
			ByteBuffer buffer;
			const auto& packet = TTraits::CreatePacketWithOverflowSize(buffer, size);

			// Act:
			auto extractResult = TTraits::Extract(packet);

			// Assert:
			EXPECT_TRUE(TTraits::IsEmpty(extractResult)) << "overflow size " << size;
		}
	}

#define PACKET_HEADER_FAILURE_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_ExtractEntity) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ExtractEntityTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_ExtractEntities) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ExtractEntitiesTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_FixedSizeStructures) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ExtractFixedSizeStructuresTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

#define PACKET_FAILURE_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_ExtractEntity) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ExtractEntityTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_ExtractEntities_Single) { \
		TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ExtractEntitiesSingleEntityTraits>(); \
	} \
	TEST(TEST_CLASS, TEST_NAME##_ExtractEntities_Last) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ExtractEntitiesMultiEntityTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

#define PACKET_SINGLE_ENTITY_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_ExtractEntity) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ExtractEntityTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_ExtractEntities) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ExtractEntitiesTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	PACKET_HEADER_FAILURE_TEST(CannotExtractFromPacketWithInvalidSize) {
		// Assert:
		AssertCannotExtractFromPacketHeaderWithNoData<TTraits>(sizeof(Packet) - 1);
	}

	PACKET_HEADER_FAILURE_TEST(CannotExtractFromPacketWithNoData) {
		// Assert:
		AssertCannotExtractFromPacketHeaderWithNoData<TTraits>(sizeof(Packet));
	}

	PACKET_FAILURE_TEST(CannotExtractFromPacketWithoutFullEntityHeader) {
		// Assert:
		for (auto size : std::vector<uint32_t>{ 1, sizeof(model::VerifiableEntity) - 1 })
			AssertCannotExtractEntitiesFromPacketWithSize<TTraits>(size);
	}

	PACKET_FAILURE_TEST(CannotExtractFromPacketWithoutFullEntityData) {
		// Assert:
		for (auto size : std::vector<uint32_t>{ sizeof(model::VerifiableEntity), sizeof(model::BlockHeaderV4) - 1 })
			AssertCannotExtractEntitiesFromPacketWithSize<TTraits>(size);
	}

	PACKET_FAILURE_TEST(CannotExtractFromPacketWhenLastEntityExpandsBeyondPacket) {
		// Arrange:
		// - create a packet wrapping a block with a transaction
		// - decrease the packet size by the size of the transaction so that the last entity expands beyond the packet
		ByteBuffer buffer;
		auto& packet = TTraits::CreatePacketWithOverflowSize(buffer, Block_Transaction_Size);
		SetTransactionAt(buffer, static_cast<uint32_t>(buffer.size() - Transaction_Size));
		packet.Size -= Transaction_Size;

		// Act:
		auto extractResult = TTraits::Extract(packet);

		// Assert:
		EXPECT_TRUE(TTraits::IsEmpty(extractResult));
	}

	PACKET_FAILURE_TEST(CannotExtractFromPacketWhenPacketExpandsBeyondLastEntity) {
		// Arrange:
		// - create a packet wrapping a block
		// - expand the buffer by the size of a transaction so it looks like the packet expands beyond the last entity
		ByteBuffer buffer;
		TTraits::CreatePacketWithOverflowSize(buffer, sizeof(model::BlockHeaderV4));
		buffer.resize(buffer.size() + Transaction_Size);
		auto& packet = reinterpret_cast<Packet&>(buffer[0]);
		packet.Size += Transaction_Size;

		// Act:
		auto extractResult = TTraits::Extract(packet);

		// Assert:
		EXPECT_TRUE(TTraits::IsEmpty(extractResult));
	}

	PACKET_SINGLE_ENTITY_TEST(CanExtractSingleBlockWithoutTransactions) {
		// Arrange: create a packet containing a block with no transactions
		ByteBuffer buffer(Block_Packet_Size);
		const auto& packet = test::SetPushBlockPacketInBuffer(buffer);

		// Act:
		auto extractResult = TTraits::Extract(packet);
		const model::Block* pBlock = nullptr;
		TTraits::Unwrap(extractResult, pBlock);

		// Assert:
		ASSERT_TRUE(!!pBlock);
		ASSERT_EQ(sizeof(model::BlockHeaderV4), pBlock->Size);
		EXPECT_EQ_MEMORY(&buffer[sizeof(Packet)], pBlock, pBlock->Size);
	}

	PACKET_SINGLE_ENTITY_TEST(IsValidPredicateHasHigherPrecedenceThanSizeCheck) {
		// Arrange: create a packet containing a block with no transactions
		ByteBuffer buffer(Block_Packet_Size);
		const auto& packet = test::SetPushBlockPacketInBuffer(buffer);

		// Act: extract and return false from the isValid predicate even though the packet has a valid size
		auto numValidCalls = 0u;
		auto extractResult = TTraits::Extract(packet, [&numValidCalls](const auto&) {
			++numValidCalls;
			return false;
		});

		// Assert: valid was called once and extraction failed
		EXPECT_EQ(1u, numValidCalls);
		EXPECT_TRUE(TTraits::IsEmpty(extractResult));
	}

	PACKET_SINGLE_ENTITY_TEST(CanExtractSingleBlockWithTransaction) {
		// Arrange: create a packet containing a block with one transaction
		ByteBuffer buffer(Block_Transaction_Packet_Size);
		const auto& packet = test::SetPushBlockPacketInBuffer(buffer, Transaction_Size);
		SetTransactionAt(buffer, static_cast<uint32_t>(buffer.size() - Transaction_Size));

		// Act:
		auto extractResult = TTraits::Extract(packet);
		const model::Block* pBlock = nullptr;
		TTraits::Unwrap(extractResult, pBlock);

		// Assert:
		ASSERT_TRUE(!!pBlock);
		ASSERT_EQ(Block_Transaction_Size, pBlock->Size);
		EXPECT_EQ_MEMORY(&buffer[sizeof(Packet)], pBlock, pBlock->Size);
	}

	namespace {
		const Packet& PrepareMultiBlockPacket(ByteBuffer& buffer) {
			// create a packet containing three blocks
			buffer.resize(Block_Transaction_Packet_Size + 2 * sizeof(model::BlockHeaderV4));
			const auto& packet = test::SetPushBlockPacketInBuffer(buffer);
			test::SetBlockAt(buffer, sizeof(Packet)); // block 1
			test::SetBlockAt(buffer, sizeof(Packet) + sizeof(model::BlockHeaderV4), Block_Transaction_Size, Transaction_Size); // block 2
			SetTransactionAt(buffer, sizeof(Packet) + 2 * sizeof(model::BlockHeaderV4)); // block 2 tx
			test::SetBlockAt(buffer, sizeof(Packet) + sizeof(model::BlockHeaderV4) + Block_Transaction_Size); // block 3
			return packet;
		}
	}

	TEST(TEST_CLASS, CanExtractMultipleBlocks_ExtractEntities) {
		// Arrange: create a packet containing three blocks
		ByteBuffer buffer;
		const auto& packet = PrepareMultiBlockPacket(buffer);

		// Act:
		auto range = ExtractEntitiesFromPacket<model::Block>(packet, test::DefaultSizeCheck<model::Block>);

		// Assert:
		ASSERT_EQ(3u, range.size());

		// - block 1
		auto iter = range.cbegin();
		const auto* pBlock = &*iter;
		size_t offset = sizeof(Packet);
		ASSERT_EQ(sizeof(model::BlockHeaderV4), pBlock->Size);
		EXPECT_EQ_MEMORY(&buffer[offset], pBlock, pBlock->Size);

		// - block 2
		pBlock = &*++iter;
		offset += sizeof(model::BlockHeaderV4);
		ASSERT_EQ(Block_Transaction_Size, pBlock->Size);
		EXPECT_EQ_MEMORY(&buffer[offset], pBlock, pBlock->Size);

		// - block 3
		pBlock = &*++iter;
		offset += Block_Transaction_Size;
		ASSERT_EQ(sizeof(model::BlockHeaderV4), pBlock->Size);
		EXPECT_EQ_MEMORY(&buffer[offset], pBlock, pBlock->Size);
	}

	TEST(TEST_CLASS, CannotExtractMultipleBlocks_ExtractEntity) {
		// Arrange: create a packet containing three blocks
		ByteBuffer buffer;
		const auto& packet = PrepareMultiBlockPacket(buffer);

		// Act:
		auto pBlock = ExtractEntityFromPacket<model::Block>(packet, test::DefaultSizeCheck<model::Block>);

		// Assert:
		EXPECT_FALSE(!!pBlock);
	}

	// endregion

	// region ExtractFixedSizeStructuresFromPacket

	TEST(TEST_CLASS, CannotExtractTransactionBatches) {
		// Arrange:
		std::vector<model::UniqueEntityPtr<mocks::MockTransaction>> transactions;
		uint32_t payloadSize = 0;
		for (auto i = 0u; i < 7; ++i) {
			auto pTransaction = mocks::CreateMockTransaction(test::RandomByte());
			payloadSize += pTransaction->Size;
			transactions.push_back(std::move(pTransaction));
		}

		auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
		auto pData = pPacket->Data();
		for (const auto& pTransaction : transactions) {
			std::memcpy(static_cast<void*>(pData), pTransaction.get(), pTransaction->Size);
			pData += pTransaction->Size;
		}

		// Act:
		auto ranges = ExtractEntityBatchesFromPacket<model::Transaction>(*pPacket, 5, [](const auto&) { return true; });

		// Assert:
		ASSERT_EQ(2, ranges.size());

		auto firstBatch = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(ranges[0]));
		ASSERT_EQ(5, firstBatch.size());
		for (auto i = 0u; i < 5; ++i)
			EXPECT_EQ_MEMORY(transactions[i].get(), firstBatch[i].get(), transactions[i]->Size);

		auto secondBatch = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(ranges[1]));
		ASSERT_EQ(2, secondBatch.size());
		for (auto i = 0u; i < 2; ++i)
			EXPECT_EQ_MEMORY(transactions[i + 5].get(), secondBatch[i].get(), transactions[i + 5]->Size);
	}

	// endregion

	// region ExtractFixedSizeStructuresFromPacket

	namespace {
		constexpr auto Fixed_Size = 62;
		using FixedSizeStructure = std::array<uint8_t, Fixed_Size>;

		void AssertCannotExtractFixedSizeStructuresFromPacketWithSize(uint32_t size) {
			// Arrange:
			ByteBuffer buffer(size);
			test::FillWithRandomData(buffer);
			auto& packet = reinterpret_cast<Packet&>(buffer[0]);
			packet.Size = size;

			// Act:
			auto range = ExtractFixedSizeStructuresFromPacket<FixedSizeStructure>(packet);

			// Assert:
			EXPECT_TRUE(range.empty()) << "packet size " << size;
		}
	}

	TEST(TEST_CLASS, CannotExtractFromPacketWithPartialStructures_FixedSizeStructures) {
		// Assert:
		AssertCannotExtractFixedSizeStructuresFromPacketWithSize(sizeof(Packet) + 1);
		AssertCannotExtractFixedSizeStructuresFromPacketWithSize(sizeof(Packet) + Fixed_Size - 1);
		AssertCannotExtractFixedSizeStructuresFromPacketWithSize(sizeof(Packet) + Fixed_Size + 1);
		AssertCannotExtractFixedSizeStructuresFromPacketWithSize(sizeof(Packet) + 3 * Fixed_Size - 1);
		AssertCannotExtractFixedSizeStructuresFromPacketWithSize(sizeof(Packet) + 3 * Fixed_Size + 1);
	}

	TEST(TEST_CLASS, CanExtractSingleStructure_FixedSizeStructures) {
		// Arrange: create a packet containing a single fixed size structure
		constexpr auto Packet_Size = sizeof(Packet) + Fixed_Size;
		ByteBuffer buffer(Packet_Size);
		test::FillWithRandomData(buffer);
		auto& packet = reinterpret_cast<Packet&>(buffer[0]);
		packet.Size = Packet_Size;

		// Act:
		auto range = ExtractFixedSizeStructuresFromPacket<FixedSizeStructure>(packet);

		// Assert:
		ASSERT_EQ(1u, range.size());
		const auto& structure = *range.cbegin();
		EXPECT_EQ_MEMORY(&buffer[sizeof(Packet)], &structure, Fixed_Size);
	}

	TEST(TEST_CLASS, CanExtractMultipleStructures_FixedSizeStructures) {
		// Arrange: create a packet containing three fixed size structures
		constexpr auto Packet_Size = sizeof(Packet) + 3 * Fixed_Size;
		ByteBuffer buffer(Packet_Size);
		test::FillWithRandomData(buffer);
		auto& packet = reinterpret_cast<Packet&>(buffer[0]);
		packet.Size = Packet_Size;

		// Act:
		auto range = ExtractFixedSizeStructuresFromPacket<FixedSizeStructure>(packet);

		// Assert:
		ASSERT_EQ(3u, range.size());

		// - structure 1
		auto iter = range.cbegin();
		size_t offset = sizeof(Packet);
		EXPECT_EQ_MEMORY(&buffer[offset], &*iter, Fixed_Size);

		// - structure 2
		++iter;
		offset += Fixed_Size;
		EXPECT_EQ_MEMORY(&buffer[offset], &*iter, Fixed_Size);

		// - structure 3
		++iter;
		offset += Fixed_Size;
		EXPECT_EQ_MEMORY(&buffer[offset], &*iter, Fixed_Size);
	}

	// endregion
}}
