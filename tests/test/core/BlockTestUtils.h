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

#pragma once
#include "TransactionTestUtils.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/model/Elements.h"
#include "catapult/model/RangeTypes.h"
#include <list>
#include <memory>
#include <vector>

namespace catapult { namespace test {

	/// Hash string of the deterministic block.
#ifdef SIGNATURE_SCHEME_NIS1
	constexpr auto Deterministic_Block_Hash_String = "80ED33530BE86977CCE8A84208F9D1C0404A12D53FFCDF442CA6FBD0F691AE35";
	constexpr auto Deterministic_Block_Hash_String_V1_Signer = "FA454A3CF1AAA33EFFE286A4B40561F3F5E70D4913E0AFE6FDD1E8A79AC360FB";
    constexpr auto Deterministic_Block_Hash_String_V2_Signer = "F6245C6287A72AF928D831209F361AF00979F9498DFD446DAD9488BCF604CC87";
#else
	constexpr auto Deterministic_Block_Hash_String = "FA454A3CF1AAA33EFFE286A4B40561F3F5E70D4913E0AFE6FDD1E8A79AC360FB"; //OLD ONE FOR V1 with NO EMBEDDED VERSION SIGNATURE
    constexpr auto Deterministic_Block_Hash_String_V1_Signer = "6469C57371ADD238BC863CCE3ED75F954D9A1ECD1A9525BEFD53C4DD1D7DDFE5";
    constexpr auto Deterministic_Block_Hash_String_V2_Signer = "F3CABB12ABC0DA02F402B9672F8AF00F5CC4FF0F58C9EAA713E31EC9491E1815";
#endif

	// region TestBlockTransactions

	/// Container of transactions for seeding a test block.
	class TestBlockTransactions {
	public:
		/// Creates block transactions from const \a transactions.
		TestBlockTransactions(const ConstTransactions& transactions);

		/// Creates block transactions from mutable \a transactions.
		TestBlockTransactions(const MutableTransactions& transactions);

		/// Creates \a numTransactions (random) block transactions.
		TestBlockTransactions(size_t numTransactions);

	public:
		/// Gets the transactions.
		const ConstTransactions& get() const;

	private:
		ConstTransactions m_transactions;
	};

	// endregion

	// region Block factory functions

	/// Generates an empty block with random signer and no transactions.
	model::UniqueEntityPtr<model::Block> GenerateEmptyRandomBlock();

	/// Generates a block with random signer and \a transactions.
	model::UniqueEntityPtr<model::Block> GenerateBlockWithTransactions(const TestBlockTransactions& transactions);

	/// Generates a block with a given \a signer and \a transactions.
	model::UniqueEntityPtr<model::Block> GenerateBlockWithTransactions(const crypto::KeyPair& signer, const TestBlockTransactions& transactions);

	/// Generates a block with \a numTransactions transactions at \a height.
	model::UniqueEntityPtr<model::Block> GenerateBlockWithTransactions(size_t numTransactions, Height height);

	/// Generates a block with \a numTransactions transactions at \a height and \a timestamp.
	model::UniqueEntityPtr<model::Block> GenerateBlockWithTransactions(size_t numTransactions, Height height, Timestamp timestamp);

	/// Generates a predefined block, i.e. this function will always return the same block.
	model::UniqueEntityPtr<model::Block> GenerateDeterministicBlock(uint32_t signerAccountVersion);

	// endregion

	/// Policy for creating an empty block.
	struct EmptyBlockPolicy {
		static auto Create() {
			return GenerateEmptyRandomBlock();
		}
	};

	/// Policy for creating a non-empty block.
	struct NonEmptyBlockPolicy {
		static auto Create() {
			auto transactions = GenerateRandomTransactions(3);
			return GenerateBlockWithTransactions(transactions);
		}
	};

	/// Creates a buffer containing \a numBlocks random blocks (all with no transactions).
	std::vector<uint8_t> CreateRandomBlockBuffer(size_t numBlocks);

	/// Copies \a blocks into an entity range.
	model::BlockRange CreateEntityRange(const std::vector<const model::Block*>& blocks);

	/// Creates a block entity range composed of \a numBlocks blocks.
	model::BlockRange CreateBlockEntityRange(size_t numBlocks);

	/// Creates \a count ranges of blocks.
	std::vector<model::BlockRange> PrepareRanges(size_t count);

	/// Counts the number of transactions in \a block.
	size_t CountTransactions(const model::Block& block);

	/// Converts \a block to a block element.
	model::BlockElement BlockToBlockElement(const model::Block& block);

	/// Converts \a block to a block element with specified generation hash (\a generationHash).
	model::BlockElement BlockToBlockElement(const model::Block& block, const GenerationHash& generationHash);

	/// Converts \a block with \a hash to a block element.
	model::BlockElement BlockToBlockElement(const model::Block& block, const Hash256& hash);

	/// Verifies that block elements \a expectedBlockElement and \a blockElement are equivalent.
	void AssertEqual(const model::BlockElement& expectedBlockElement, const model::BlockElement& blockElement);
}}
