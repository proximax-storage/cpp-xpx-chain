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
	constexpr auto Deterministic_Block_Hash_String = "4D4C0D925A79FA397634638745301E5AC255C279868A7E19FBCEDC25E80DECAA";
#else
	constexpr auto Deterministic_Block_Hash_String = "CEEFD9DDCCD3547AA5FF79EDBD6DBF07D5AD9EC8620E8E610DE62206CAA71704";
#endif

	/// Generates an empty block with random signer and no transactions.
	std::unique_ptr<model::Block> GenerateEmptyRandomBlock();

	/// Generates a block with random signer and given \a transactions.
	std::unique_ptr<model::Block> GenerateRandomBlockWithTransactions(const ConstTransactions& transactions);

	/// Generates a block with random signer and given \a transactions.
	std::unique_ptr<model::Block> GenerateRandomBlockWithTransactions(const MutableTransactions& transactions);

	/// Generates a block with a given \a signer and given \a transactions.
	std::unique_ptr<model::Block> GenerateBlockWithTransactions(const crypto::KeyPair& signer, const ConstTransactions& transactions);

	/// Generates a block with a given \a signer and given \a transactions.
	std::unique_ptr<model::Block> GenerateBlockWithTransactions(const crypto::KeyPair& signer, const MutableTransactions& transactions);

	/// Generates a block with \a numTransactions transactions.
	std::unique_ptr<model::Block> GenerateBlockWithTransactions(size_t numTransactions);

	/// Generates a block with \a numTransactions transactions at \a height.
	std::unique_ptr<model::Block> GenerateBlockWithTransactionsAtHeight(size_t numTransactions, size_t height);

	/// Generates a block with \a numTransactions transactions at \a height.
	std::unique_ptr<model::Block> GenerateBlockWithTransactionsAtHeight(size_t numTransactions, Height height);

	/// Generates a block with transactions at \a height.
	std::unique_ptr<model::Block> GenerateBlockWithTransactionsAtHeight(Height height);

	/// Generates a block with \a numTransactions transactions at \a height and \a timestamp.
	std::unique_ptr<model::Block> GenerateBlockWithTransactions(size_t numTransactions, Height height, Timestamp timestamp);

	/// Generates a block with no transactions and \a height that can be verified.
	std::unique_ptr<model::Block> GenerateVerifiableBlockAtHeight(Height height);

	/// Generates a block with no transactions and \a height that cannot be verified.
	std::unique_ptr<model::Block> GenerateNonVerifiableBlockAtHeight(Height height);

	/// Generates a predefined block, i.e. this function will always return the same block.
	std::unique_ptr<model::Block> GenerateDeterministicBlock();

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
			return GenerateRandomBlockWithTransactions(transactions);
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

	/// Creates a copy of \a block.
	std::unique_ptr<model::Block> CopyBlock(const model::Block& block);

	/// Converts \a block with \a hash to a block element.
	model::BlockElement BlockToBlockElement(const model::Block& block, const Hash256& hash);

	/// Converts \a block to a block element.
	model::BlockElement BlockToBlockElement(const model::Block& block);

	/// Verifies that block elements \a expectedBlockElement and \a blockElement are equivalent.
	void AssertEqual(const model::BlockElement& expectedBlockElement, const model::BlockElement& blockElement);

	/// Signs \a block as \a signer and calculates the block transactions hash.
	/// \note All data is assumed to be present and valid.
	void SignBlock(const crypto::KeyPair& signer, model::Block& block);
}}
