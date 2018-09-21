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

#include "BlockUtils.h"
#include "catapult/crypto/Hashes.h"
#include "catapult/crypto/MerkleHashBuilder.h"
#include "catapult/crypto/Signer.h"
#include "catapult/utils/MemoryUtils.h"
#include "catapult/utils/TimeSpan.h"
#include <cstring>

namespace catapult { namespace model {

	namespace {
		RawBuffer BlockDataBuffer(const Block& block) {
			return {
				reinterpret_cast<const uint8_t*>(&block) + VerifiableEntity::Header_Size,
				sizeof(Block) - VerifiableEntity::Header_Size
			};
		}
	}

	// region hashes

	void CalculateBlockTransactionsHash(const std::vector<const TransactionInfo*>& transactionInfos, Hash256& blockTransactionsHash) {
		crypto::MerkleHashBuilder builder;
		for (const auto* pTransactionInfo : transactionInfos)
			builder.update(pTransactionInfo->MerkleComponentHash);

		builder.final(blockTransactionsHash);
	}

	Hash256 CalculateGenerationHash(const Hash256& previousGenerationHash, const Key& publicKey) {
		Hash256 hash;
		crypto::Sha3_256_Builder sha3;
		sha3.update(previousGenerationHash);
		sha3.update(publicKey);
		sha3.final(hash);
		return hash;
	}

	// endregion

	// region sign / verify

	void SignBlockHeader(const crypto::KeyPair& signer, Block& block) {
		crypto::Sign(signer, BlockDataBuffer(block), block.Signature);
	}

	bool VerifyBlockHeaderSignature(const Block& block) {
		return crypto::Verify(block.Signer, BlockDataBuffer(block), block.Signature);
	}

	// endregion

	// region create block

	namespace {
		void CopyEntity(uint8_t* pDestination, const VerifiableEntity& source) {
			std::memcpy(pDestination, &source, source.Size);
		}

		template<typename TContainer>
		void CopyTransactions(uint8_t* pDestination, const TContainer& transactions) {
			for (const auto& pTransaction : transactions) {
				CopyEntity(pDestination, *pTransaction);
				pDestination += pTransaction->Size;
			}
		}

		template<typename TContainer>
		size_t CalculateTotalSize(const TContainer& transactions) {
			size_t totalTransactionsSize = 0;
			for (const auto& pTransaction : transactions)
				totalTransactionsSize += pTransaction->Size;

			return totalTransactionsSize;
		}

	    // The cumulative difficulty value is derived from the base target value, using the formula:
		// Dcb = Dpb + 2^64 / Tb
		// where:
		// Dcb is the difficulty of the current block
		// Dpb is the difficulty of the previous block
		// Tb is the base target value for the current block
		Difficulty CalculateCumulativeDifficulty(
				const BlockTarget& target,
				const Difficulty& previousBlockDifficulty) {
			BlockTarget TWO_TO_64{1};
			TWO_TO_64 <<= 64;

			if (!target) {
				CATAPULT_THROW_RUNTIME_ERROR("Can't calculate cumulative difficulty with zero target");
			}

			auto res = BlockTarget{previousBlockDifficulty.unwrap()} + TWO_TO_64 / target;
			return Difficulty{res.convert_to<uint64_t>()};
		}

		template<typename TContainer>
		std::unique_ptr<Block> CreateBlockT(
				const PreviousBlockContext& previousBlockContext,
				const model::BlockHitContext& hitContext,
				NetworkIdentifier networkIdentifier,
				const Key& signerPublicKey,
				const TContainer& transactions) {
			auto size = sizeof(Block) + CalculateTotalSize(transactions);
			auto pBlock = utils::MakeUniqueWithSize<Block>(size);

			auto& block = *pBlock;
			block.Size = static_cast<uint32_t>(size);
			block.Version = MakeVersion(networkIdentifier, 3);
			block.Type = Entity_Type_Block;
			block.Signer = signerPublicKey;
			block.Signature = Signature{}; // zero the signature
			block.Timestamp = Timestamp();
			block.Height = previousBlockContext.BlockHeight + Height(1);
			block.PreviousBlockHash = previousBlockContext.BlockHash;
			block.BaseTarget = hitContext.BaseTarget;
			block.CumulativeDifficulty = CalculateCumulativeDifficulty(hitContext.BaseTarget, previousBlockContext.Difficulty);
			block.EffectiveBalance = hitContext.EffectiveBalance;

			// append all the transactions
			auto pDestination = reinterpret_cast<uint8_t*>(block.TransactionsPtr());
			CopyTransactions(pDestination, transactions);
			return pBlock;
		}
	}

	std::unique_ptr<Block> CreateBlock(
			const PreviousBlockContext& previousBlockContext,
			const model::BlockHitContext& hitContext,
			NetworkIdentifier networkIdentifier,
			const Key& signerPublicKey,
			const Transactions& transactions) {
		return CreateBlockT(previousBlockContext, hitContext, networkIdentifier, signerPublicKey, transactions);
	}

	// endregion
}}
