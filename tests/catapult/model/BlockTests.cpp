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

#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/TransactionContainerTestUtils.h"
#include "tests/test/core/mocks/MockTransaction.h"

namespace catapult { namespace model {

#define TEST_CLASS BlockTests

	TEST(TEST_CLASS, BlockHeaderHasExpectedSize) {
		// Arrange:
		auto expectedSize =
				sizeof(VerifiableEntity) // base
				+ sizeof(uint64_t) // height
				+ sizeof(uint64_t) // timestamp
				+ sizeof(uint64_t) // difficulty
				+ sizeof(uint32_t) // fee multiplier
				+ Hash256_Size // previous block hash
				+ Hash256_Size // block transactions hash
				+ Hash256_Size // block receipts hash
				+ Hash256_Size // state hash
				+ Key_Size // beneficiary
				+ sizeof(uint32_t) // fee interest
				+ sizeof(uint32_t); // fee interest divisibility

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(BlockHeader));
		EXPECT_EQ(106u + 196u, sizeof(BlockHeader));

		EXPECT_EQ(sizeof(BlockHeader), sizeof(decltype(Block()))); // use decltype to bypass lint rule
	}

	TEST(TEST_CLASS, BlockHeaderV4HasExpectedSize) {
		// Arrange:
		auto expectedSize =
				sizeof(VerifiableEntity) // base
				+ sizeof(uint64_t) // height
				+ sizeof(uint64_t) // timestamp
				+ sizeof(uint64_t) // difficulty
				+ sizeof(uint32_t) // fee multiplier
				+ Hash256_Size // previous block hash
				+ Hash256_Size // block transactions hash
				+ Hash256_Size // block receipts hash
				+ Hash256_Size // state hash
				+ Key_Size // beneficiary
				+ sizeof(uint32_t) // fee interest
				+ sizeof(uint32_t) // fee interest divisibility
				+ sizeof(int64_t) // committee round
				+ sizeof(uint64_t) // committee phase time
				+ sizeof(uint32_t); // transaction payload size

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(BlockHeaderV4));
		EXPECT_EQ(106u + 216u, sizeof(BlockHeaderV4));
	}

	// region test utils

	namespace {
		model::UniqueEntityPtr<Block> CreateBlockWithTransactions(size_t numTransactions = 3) {
			auto transactions = test::GenerateRandomTransactions(numTransactions);
			return test::GenerateBlockWithTransactions(transactions);
		}

		model::UniqueEntityPtr<Block> CreateBlockWithReportedSize(uint32_t size, size_t numTransactions = 3) {
			auto pBlock = CreateBlockWithTransactions(numTransactions);
			pBlock->Size = size;
			return pBlock;
		}

		mocks::MockTransaction& GetSecondTransaction(Block& block) {
			uint8_t* pBytes = reinterpret_cast<uint8_t*>(block.TransactionsPtr());
			return *reinterpret_cast<mocks::MockTransaction*>(pBytes + block.TransactionsPtr()->Size);
		}
	}

	// endregion

	// region transactions

	namespace {
		using ConstTraits = test::ConstTraitsT<Block>;
		using NonConstTraits = test::NonConstTraitsT<Block>;
	}

#define DATA_POINTER_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Const) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ConstTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_NonConst) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<NonConstTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	DATA_POINTER_TEST(TransactionsAreInaccessibleWhenBlockHasNoTransactions) {
		// Arrange:
		auto pBlock = CreateBlockWithReportedSize(sizeof(BlockHeader));
		auto& accessor = TTraits::GetAccessor(*pBlock);

		// Act + Assert:
		EXPECT_FALSE(!!accessor.TransactionsPtr());
		EXPECT_EQ(0u, test::CountTransactions(accessor.Transactions()));
	}

	DATA_POINTER_TEST(TransactionsAreAccessibleWhenBlockHasTransactions) {
		// Arrange:
		auto pBlock = CreateBlockWithTransactions();
		const auto* pBlockEnd = test::AsVoidPointer(reinterpret_cast<const BlockHeaderV4*>(pBlock.get()) + 1);
		auto& accessor = TTraits::GetAccessor(*pBlock);

		// Act + Assert:
		EXPECT_EQ(pBlockEnd, accessor.TransactionsPtr());
		EXPECT_EQ(3u, test::CountTransactions(accessor.Transactions()));
	}

	// endregion

	// region GetTransactionPayloadSize

	TEST(TEST_CLASS, GetTransactionPayloadSizeReturnsCorrectPayloadSize) {
		// Arrange:
		BlockHeaderV4 header;
		header.Size = sizeof(BlockHeaderV4);
		header.Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, model::BlockHeader::Current_Version);
		header.TransactionPayloadSize = 123u;

		// Act:
		auto payloadSize = GetTransactionPayloadSize(header);

		// Assert:
		EXPECT_EQ(123u, payloadSize);
	}

	// endregion

	namespace {
		bool IsSizeValid(const Block& block, mocks::PluginOptionFlags options = mocks::PluginOptionFlags::Default) {
			auto registry = mocks::CreateDefaultTransactionRegistry(options);
			return IsSizeValid(block, registry);
		}
	}

	// region IsSizeValid - no transactions

	TEST(TEST_CLASS, SizeInvalidWhenReportedSizeIsZero) {
		// Arrange:
		auto pBlock = CreateBlockWithReportedSize(0, 0);

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pBlock));
	}

	TEST(TEST_CLASS, SizeInvalidWhenReportedSizeIsLessThanHeaderSize) {
		// Arrange:
		auto pBlock = CreateBlockWithReportedSize(sizeof(BlockHeader) - 1, 0);

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pBlock));
	}

	TEST(TEST_CLASS, SizeValidWhenReportedSizeIsEqualToHeaderSize) {
		// Arrange:
		auto pBlock = CreateBlockWithReportedSize(sizeof(BlockHeaderV4), 0);

		// Act + Assert:
		EXPECT_TRUE(IsSizeValid(*pBlock));
	}

	// endregion

	// region IsSizeValid - invalid inner tx sizes

	TEST(TEST_CLASS, SizeInvalidWhenTransactionPayloadSizeInvalid) {
		auto pBlock = CreateBlockWithReportedSize(sizeof(BlockHeaderV4));
		pBlock->setTransactionPayloadSize(std::numeric_limits<uint32_t>::max());

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pBlock));
	}

	TEST(TEST_CLASS, SizeInvalidWhenAnyTransactionHasPartialHeader) {
		// Arrange: create a block with 1 extra byte (which should be interpreted as a partial tx header)
		auto pBlock = CreateBlockWithReportedSize(sizeof(BlockHeader) + 1);

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pBlock));
	}

	TEST(TEST_CLASS, SizeInvalidWhenAnyTransactionHasInvalidSize) {
		// Arrange:
		auto pBlock = CreateBlockWithTransactions();
		GetSecondTransaction(*pBlock).Data.Size = 1;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pBlock));
	}

	TEST(TEST_CLASS, SizeInvalidWhenAnyTransactionHasZeroSize) {
		// Arrange:
		auto pBlock = CreateBlockWithTransactions();
		GetSecondTransaction(*pBlock).Size = 0;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pBlock));
	}

	TEST(TEST_CLASS, SizeInvalidWhenAnyInnerTransactionExpandsBeyondBuffer) {
		// Arrange:
		auto pBlock = CreateBlockWithTransactions();
		GetSecondTransaction(*pBlock).Size = pBlock->Size - pBlock->TransactionsPtr()->Size + 1;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pBlock));
	}

	// endregion

	// region IsSizeValid - invalid inner tx types

	TEST(TEST_CLASS, SizeInvalidWhenAnyTransactionHasUnknownType) {
		// Arrange:
		auto pBlock = CreateBlockWithTransactions();
		GetSecondTransaction(*pBlock).Type = static_cast<EntityType>(-1);

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pBlock));
	}

	TEST(TEST_CLASS, SizeValidWhenAnyTransactionDoesNotSupportEmbedding) {
		// Arrange:
		auto pBlock = CreateBlockWithTransactions();

		// Act + Assert:
		EXPECT_TRUE(IsSizeValid(*pBlock, mocks::PluginOptionFlags::Not_Embeddable));
	}

	// endregion

	// region IsSizeValid - valid transactions

	TEST(TEST_CLASS, SizeValidWhenReportedSizeIsEqualToHeaderSizePlusTransactionsSize) {
		// Arrange:
		auto pBlock = CreateBlockWithTransactions();

		// Act + Assert:
		EXPECT_TRUE(IsSizeValid(*pBlock));
	}

	// endregion

	// region IsSizeValid - cosignatures

	TEST(TEST_CLASS, SizeInvalidWhenCosignaturesHaveInvalidSize) {
		// Arrange:
		auto pBlock = CreateBlockWithTransactions();
		pBlock->Size += sizeof(model::Cosignature) - 1;

		// Act + Assert:
		EXPECT_FALSE(IsSizeValid(*pBlock));
	}

	TEST(TEST_CLASS, SizeValidWhenCosignaturesHaveValidSize) {
		// Arrange:
		auto pBlock = CreateBlockWithTransactions();
		pBlock->Size += sizeof(model::Cosignature);

		// Act + Assert:
		EXPECT_TRUE(IsSizeValid(*pBlock));
	}

	// endregion
}}
