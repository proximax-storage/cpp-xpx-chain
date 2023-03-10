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
#include "FeeUtils.h"
#include "catapult/crypto/MerkleHashBuilder.h"
#include "catapult/crypto/Signer.h"
#include "catapult/model/TransactionFeeCalculator.h"

namespace catapult { namespace model {

	namespace {
		RawBuffer BlockDataBuffer(const Block& block) {
			return {
				reinterpret_cast<const uint8_t*>(&block) + VerifiableEntity::Header_Size,
				block.GetHeaderSize() - VerifiableEntity::Header_Size
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

	GenerationHash CalculateGenerationHash(const GenerationHash& previousGenerationHash, const Key& publicKey) {
		GenerationHash generationHash;
		crypto::GenerationHash_Builder hasher;
		hasher.update(previousGenerationHash);
		hasher.update(publicKey);
		hasher.final(generationHash);
		return generationHash;
	}

	// endregion

	// region sign / verify

	void SignBlockHeader(const crypto::KeyPair& signer, Block& block) {
		crypto::Sign(signer, BlockDataBuffer(block), block.Signature);
	}

	void CosignBlockHeader(const crypto::KeyPair& signer, Block& block, Signature& signature) {
		crypto::Sign(signer, BlockDataBuffer(block), signature);
	}

	bool VerifyBlockHeaderSignature(const Block& block) {
		return crypto::Verify(block.Signer, BlockDataBuffer(block), block.Signature);
	}

	bool VerifyBlockHeaderCosignature(const Block& block, const model::Cosignature& cosignature) {
		return crypto::Verify(cosignature.Signer, BlockDataBuffer(block), cosignature.Signature);
	}

	// endregion

	// region fees

	BlockTransactionsInfo CalculateBlockTransactionsInfo(const Block& block,
														 const TransactionFeeCalculator& transactionFeeCalculator) {
		BlockTransactionsInfo blockTransactionsInfo;
		for (const auto& transaction : block.Transactions()) {
			auto transactionFee = transactionFeeCalculator.calculateTransactionFee(block.FeeMultiplier, transaction, block.FeeInterest, block.FeeInterestDenominator);
			blockTransactionsInfo.TotalFee = blockTransactionsInfo.TotalFee + transactionFee;
			++blockTransactionsInfo.Count;
		}

		return blockTransactionsInfo;
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

		template<typename TContainer>
		UniqueEntityPtr<Block> CreateBlockT(
				const PreviousBlockContext& context,
				NetworkIdentifier networkIdentifier,
				const Key& signerPublicKey,
				const TContainer& transactions,
				VersionType version) {
			auto transactionPayloadSize = CalculateTotalSize(transactions);
			auto headerSize = (version > 3) ? sizeof(BlockHeaderV4) : sizeof(BlockHeader);
			auto size = headerSize + transactionPayloadSize;
			auto pBlock = utils::MakeUniqueWithSize<Block>(size);
			std::memset(static_cast<void*>(pBlock.get()), 0, headerSize);
			pBlock->Size = static_cast<uint32_t>(size);
			pBlock->Version = MakeVersion(networkIdentifier, version);
			pBlock->setTransactionPayloadSize(transactionPayloadSize);

			pBlock->Signer = signerPublicKey;
			pBlock->Type = Entity_Type_Block;

			pBlock->Height = context.BlockHeight + Height(1);
			pBlock->Difficulty = Difficulty();
			pBlock->PreviousBlockHash = context.BlockHash;

			// append all the transactions
			auto pDestination = reinterpret_cast<uint8_t*>(pBlock->TransactionsPtr());
			CopyTransactions(pDestination, transactions);
			return pBlock;
		}
	}

	UniqueEntityPtr<Block> CreateBlock(
			const PreviousBlockContext& context,
			NetworkIdentifier networkIdentifier,
			const Key& signerPublicKey,
			const Transactions& transactions,
			VersionType version) {
		return CreateBlockT(context, networkIdentifier, signerPublicKey, transactions, version);
	}

	UniqueEntityPtr<Block> StitchBlock(const Block& blockHeader, const Transactions& transactions) {
		auto transactionPayloadSize = CalculateTotalSize(transactions);
		auto headerSize = blockHeader.GetHeaderSize();
		auto size = headerSize + transactionPayloadSize;
		auto pBlock = utils::MakeUniqueWithSize<Block>(size);
		std::memcpy(static_cast<void*>(pBlock.get()), &blockHeader, headerSize);
		pBlock->Size = static_cast<uint32_t>(size);
		pBlock->setTransactionPayloadSize(transactionPayloadSize);

		// append all the transactions
		auto pDestination = reinterpret_cast<uint8_t*>(pBlock->TransactionsPtr());
		CopyTransactions(pDestination, transactions);
		return pBlock;
	}

	// endregion
}}
