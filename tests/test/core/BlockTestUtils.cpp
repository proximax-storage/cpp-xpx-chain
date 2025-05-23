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

#include "BlockTestUtils.h"
#include "BlockStatementTestUtils.h"
#include "EntityTestUtils.h"
#include "sdk/src/extensions/BlockExtensions.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/utils/HexParser.h"
#include "tests/test/nodeps/TestConstants.h"

namespace catapult { namespace test {

	// region TestBlockTransactions

	TestBlockTransactions::TestBlockTransactions(const ConstTransactions& transactions) : m_transactions(transactions)
	{}

	TestBlockTransactions::TestBlockTransactions(const MutableTransactions& transactions) : TestBlockTransactions(MakeConst(transactions))
	{}

	TestBlockTransactions::TestBlockTransactions(size_t numTransactions)
			: TestBlockTransactions(GenerateRandomTransactions(numTransactions))
	{}

	const ConstTransactions& TestBlockTransactions::get() const {
		return m_transactions;
	}

	// endregion

	// region Block factory functions

	namespace {
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;

		void RandomizeBlock(model::Block& block) {
			auto difficultyRange = (Difficulty::Max() - Difficulty::Min()).unwrap();
			auto difficultyAdjustment = difficultyRange * RandomByte() / std::numeric_limits<uint8_t>::max();

			block.Height = GenerateRandomValue<Height>();
			block.Timestamp = GenerateRandomValue<Timestamp>();
			block.Difficulty = Difficulty::Min() + Difficulty::Unclamped(difficultyAdjustment);
			block.FeeMultiplier = GenerateRandomValue<BlockFeeMultiplier>();
			FillWithRandomData(block.PreviousBlockHash);
			FillWithRandomData(block.BlockTransactionsHash);
			FillWithRandomData(block.BlockReceiptsHash);
			FillWithRandomData(block.StateHash);
			FillWithRandomData(block.Beneficiary);
		}

		struct TestBlockOptions {
		public:
			explicit TestBlockOptions(const crypto::KeyPair& signer) : Signer(signer)
			{}

		public:
			const crypto::KeyPair& Signer;
			catapult::Height Height;
			catapult::Timestamp Timestamp;
		};

		model::UniqueEntityPtr<model::Block> GenerateBlock(
				const TestBlockTransactions& transactions,
				const TestBlockOptions& options,
				size_t numCosignatures = 0,
				VersionType version = model::Block::Current_Version) {
			model::PreviousBlockContext context;
			auto pBlock = model::CreateBlock(context, Network_Identifier, options.Signer.publicKey(), transactions.get(), version);
			RandomizeBlock(*pBlock);

			if (Height() != options.Height)
				pBlock->Height = options.Height;

			if (Timestamp() != options.Timestamp) {
				pBlock->Timestamp = options.Timestamp;
				for (auto& transaction : pBlock->Transactions())
					transaction.Deadline = options.Timestamp + Timestamp(1);
			}

			if (!numCosignatures)
				return pBlock;

			auto blockSize = pBlock->Size + numCosignatures * sizeof(model::Cosignature);
			auto pBlockWithCosignatures = utils::MakeUniqueWithSize<model::Block>(blockSize);
			auto* pData = reinterpret_cast<uint8_t*>(pBlockWithCosignatures.get());
			std::memcpy(pData, pBlock.get(), pBlock->Size);
			pBlockWithCosignatures->Size = blockSize;

			auto pCosignature = pBlockWithCosignatures->CosignaturesPtr();
			for (auto i = 0u; i < pBlockWithCosignatures->CosignaturesCount(); ++i, ++pCosignature) {
				FillWithRandomData(pCosignature->Signer);
				FillWithRandomData(pCosignature->Signature);
			}

			return pBlockWithCosignatures;
		}
	}

	model::UniqueEntityPtr<model::Block> GenerateEmptyRandomBlock(VersionType version) {
		return GenerateBlockWithTransactions(0, version);
	}

	model::UniqueEntityPtr<model::Block> GenerateBlockWithTransactions(const TestBlockTransactions& transactions, VersionType version) {
		return GenerateBlock(transactions, TestBlockOptions(GenerateKeyPair()), 0, version);
	}

	model::UniqueEntityPtr<model::Block> GenerateBlockWithTransactions(const crypto::KeyPair& signer, const TestBlockTransactions& transactions) {
		return GenerateBlock(transactions, TestBlockOptions(signer));
	}

	model::UniqueEntityPtr<model::Block> GenerateBlockWithTransactions(size_t numTransactions, Height height) {
		auto signer = GenerateKeyPair();
		auto blockOptions = TestBlockOptions(signer);
		blockOptions.Height = height;
		return GenerateBlock(numTransactions, blockOptions);
	}

	model::UniqueEntityPtr<model::Block> GenerateBlockWithTransactions(size_t numTransactions, Height height, Timestamp timestamp) {
		auto signer = GenerateKeyPair();
		auto blockOptions = TestBlockOptions(signer);
		blockOptions.Height = height;
		blockOptions.Timestamp = timestamp;
		return GenerateBlock(numTransactions, blockOptions);
	}

	model::UniqueEntityPtr<model::Block> GenerateBlockWithTransactionsAndCosignatures(const TestBlockTransactions& transactions, size_t numCosignatures) {
		return GenerateBlock(transactions, TestBlockOptions(GenerateKeyPair()), numCosignatures);
	}

	model::UniqueEntityPtr<model::Block> GenerateBlockWithTransactionsAndCosignatures(size_t numTransactions, Height height, size_t numCosignatures) {
		auto signer = GenerateKeyPair();
		auto blockOptions = TestBlockOptions(signer);
		blockOptions.Height = height;
		return GenerateBlock(numTransactions, blockOptions, numCosignatures);
	}

	model::UniqueEntityPtr<model::Block> GenerateDeterministicBlock() {
		auto keyPair = crypto::KeyPair::FromString("A41BE076B942D915EA3330B135D35C5A959A2DCC50BBB393C6407984D4A3B564");
		ConstTransactions transactions;
		transactions.push_back(GenerateDeterministicTransaction());
		auto pBlock = GenerateBlockWithTransactions(keyPair, transactions);
		pBlock->Signer = keyPair.publicKey();
		pBlock->Height = Height(12345);
		pBlock->Timestamp = Timestamp(54321);
		pBlock->Difficulty = Difficulty(123'456'789'123'456);
		pBlock->FeeMultiplier = BlockFeeMultiplier(8462);
		pBlock->PreviousBlockHash = { { 123 } };
		pBlock->BlockReceiptsHash = { { 55 } };
		pBlock->StateHash = { { 242, 111 } };
		pBlock->Beneficiary = { { 77, 99, 88 } };

		auto generationHash = utils::ParseByteArray<GenerationHash>(test::Deterministic_Network_Generation_Hash_String);
		extensions::BlockExtensions(generationHash).signFullBlock(keyPair, *pBlock);
		return pBlock;
	}

	// endregion

	std::vector<uint8_t> CreateRandomBlockBuffer(size_t numBlocks) {
		constexpr auto Entity_Size = sizeof(model::BlockHeader);
		auto buffer = GenerateRandomVector(numBlocks * Entity_Size);
		for (auto i = 0u; i < numBlocks; ++i) {
			auto& block = reinterpret_cast<model::Block&>(buffer[i * Entity_Size]);
			block.Size = Entity_Size;
			block.Type = model::Entity_Type_Block;
		}

		return buffer;
	}

	model::BlockRange CreateEntityRange(const std::vector<const model::Block*>& blocks) {
		return CreateEntityRange<model::Block>(blocks);
	}

	model::BlockRange CreateBlockEntityRange(size_t numBlocks) {
		auto buffer = CreateRandomBlockBuffer(numBlocks);
		return model::BlockRange::CopyFixed(buffer.data(), numBlocks);
	}

	std::vector<model::BlockRange> PrepareRanges(size_t count) {
		std::vector<model::BlockRange> ranges;
		for (auto i = 0u; i < count; ++i)
			ranges.push_back(CreateBlockEntityRange(3));

		return ranges;
	}

	size_t CountTransactions(const model::Block& block) {
		auto count = std::distance(block.Transactions().cbegin(), block.Transactions().cend());
		return static_cast<size_t>(count);
	}

	model::BlockElement BlockToBlockElement(const model::Block& block) {
		return BlockToBlockElement(block, GetDefaultGenerationHash());
	}

	model::BlockElement BlockToBlockElement(const model::Block& block, const GenerationHash& generationHash) {
		return extensions::BlockExtensions(generationHash).convertBlockToBlockElement(block, generationHash);
	}

	model::BlockElement BlockToBlockElement(const model::Block& block, const Hash256& hash) {
		auto blockElement = BlockToBlockElement(block);
		blockElement.EntityHash = hash;
		for (auto& transactionElement : blockElement.Transactions) {
			auto pAddresses = std::make_shared<model::UnresolvedAddressSet>();
			pAddresses->emplace(GenerateRandomUnresolvedAddress());
			transactionElement.OptionalExtractedAddresses = pAddresses;
		}

		// add random data to ensure it is roundtripped correctly
		blockElement.SubCacheMerkleRoots = GenerateRandomDataVector<Hash256>(3);
		return blockElement;
	}

	namespace {
		void AssertTransactionHashes(
				const std::vector<model::TransactionElement>& expectedElements,
				const std::vector<model::TransactionElement>& actualElements) {
			ASSERT_EQ(expectedElements.size(), actualElements.size());

			auto iter = actualElements.cbegin();
			auto i = 0u;
			for (const auto& expected : expectedElements) {
				auto message = "failed at transaction " + std::to_string(++i);
				EXPECT_EQ(expected.EntityHash, iter->EntityHash) << message;
				EXPECT_EQ(expected.MerkleComponentHash, iter->MerkleComponentHash) << message;
				++iter;
			}
		}
	}

	void AssertEqual(const model::BlockElement& expectedBlockElement, const model::BlockElement& blockElement) {
		// Assert:
		EXPECT_EQ(expectedBlockElement.Block.Signature, blockElement.Block.Signature);
		EXPECT_EQ(expectedBlockElement.Block, blockElement.Block);
		EXPECT_EQ(expectedBlockElement.EntityHash, blockElement.EntityHash);
		EXPECT_EQ(expectedBlockElement.GenerationHash, blockElement.GenerationHash);
		EXPECT_EQ(expectedBlockElement.Block.StateHash, blockElement.Block.StateHash);
		EXPECT_EQ(expectedBlockElement.SubCacheMerkleRoots, blockElement.SubCacheMerkleRoots);
		AssertTransactionHashes(expectedBlockElement.Transactions, blockElement.Transactions);

		if (!expectedBlockElement.OptionalStatement) {
			EXPECT_FALSE(!!blockElement.OptionalStatement);
			return;
		}

		ASSERT_TRUE(!!blockElement.OptionalStatement);
		AssertEqual(*expectedBlockElement.OptionalStatement, *blockElement.OptionalStatement);
	}
}}
